#!/usr/bin/python2
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""EC-3PO EC Interpreter

interpreter provides the interpretation layer between the EC UART and the user.
It recives commands through its command pipe, formats the commands for the EC,
and sends the command to the EC.  It also presents data from the EC to either be
displayed via the interactive console interface, or some other consumer.  It
additionally supports automatic command retrying if the EC drops a character in
a command.
"""
from __future__ import print_function
import binascii
from chromite.lib import cros_logging as logging
import os
import Queue
import select


COMMAND_RETRIES = 3  # Number of attempts to retry a command.
EC_MAX_READ = 1024  # Max bytes to read at a time from the EC.
EC_SYN = '\xec'  # Byte indicating EC interrogation.
EC_ACK = '\xc0'  # Byte representing correct EC response to interrogation.
EC_INTERROGATION_TIMEOUT = 0.1  # Maximum number of seconds to wait for a
                                # response to an interrogation.


class Interpreter(object):
  """Class which provides the interpretation layer between the EC and user.

  This class essentially performs all of the intepretation for the EC and the
  user.  It handles all of the automatic command retrying as well as the
  formation of commands for EC images which support that.

  Attributes:
    ec_uart_pty: A string representing the EC UART to connect to.
    cmd_pipe: A multiprocessing.Connection object which represents the
      Interpreter side of the command pipe.  This must be a bidirectional pipe.
      Commands and responses will utilize this pipe.
    dbg_pipe: A multiprocessing.Connection object which represents the
      Interpreter side of the debug pipe. This must be a unidirectional pipe
      with write capabilities.  EC debug output will utilize this pipe.
    cmd_retries: An integer representing the number of attempts the console
      should retry commands if it receives an error.
    log_level: An integer representing the numeric value of the log level.
    inputs: A list of objects that the intpreter selects for reading.
      Initially, these are the EC UART and the command pipe.
    outputs: A list of objects that the interpreter selects for writing.
    ec_cmd_queue: A FIFO queue used for sending commands down to the EC UART.
    last_cmd: A string that represents the last command sent to the EC.  If an
      error is encountered, the interpreter will attempt to retry this command
      up to COMMAND_RETRIES.
    enhanced_ec: A boolean indicating if the EC image that we are currently
      communicating with is enhanced or not.  Enhanced EC images will support
      packed commands and host commands over the UART.  This defaults to False
      and is changed depending on the result of an interrogation.
    interrogating: A boolean indicating if we are in the middle of interrogating
      the EC.
  """
  def __init__(self, ec_uart_pty, cmd_pipe, dbg_pipe, log_level=logging.INFO):
    """Intializes an Interpreter object with the provided args.

    Args:
      ec_uart_pty: A string representing the EC UART to connect to.
      cmd_pipe: A multiprocessing.Connection object which represents the
        Interpreter side of the command pipe.  This must be a bidirectional
        pipe.  Commands and responses will utilize this pipe.
      dbg_pipe: A multiprocessing.Connection object which represents the
        Interpreter side of the debug pipe. This must be a unidirectional pipe
        with write capabilities.  EC debug output will utilize this pipe.
      cmd_retries: An integer representing the number of attempts the console
        should retry commands if it receives an error.
      log_level: An optional integer representing the numeric value of the log
        level.  By default, the log level will be logging.INFO (20).
    """
    self.ec_uart_pty = open(ec_uart_pty, 'a+')
    self.cmd_pipe = cmd_pipe
    self.dbg_pipe = dbg_pipe
    self.cmd_retries = COMMAND_RETRIES
    self.log_level = log_level
    self.inputs = [self.ec_uart_pty, self.cmd_pipe]
    self.outputs = []
    self.ec_cmd_queue = Queue.Queue()
    self.last_cmd = ''
    self.enhanced_ec = False
    self.interrogating = False

  def __str__(self):
    """Show internal state of the Interpreter object.

    Returns:
      A string that shows the values of the attributes.
    """
    string = []
    string.append('%r' % self)
    string.append('ec_uart_pty: %s' % self.ec_uart_pty)
    string.append('cmd_pipe: %r' % self.cmd_pipe)
    string.append('dbg_pipe: %r' % self.dbg_pipe)
    string.append('cmd_retries: %d' % self.cmd_retries)
    string.append('log_level: %d' % self.log_level)
    string.append('inputs: %r' % self.inputs)
    string.append('outputs: %r' % self.outputs)
    string.append('ec_cmd_queue: %r' % self.ec_cmd_queue)
    string.append('last_cmd: \'%s\'' % self.last_cmd)
    string.append('enhanced_ec: %r' % self.enhanced_ec)
    string.append('interrogating: %r' % self.interrogating)
    return '\n'.join(string)

  def EnqueueCmd(self, command):
    """Enqueue a command to be sent to the EC UART.

    Args:
      command: A string which contains the command to be sent.
    """
    self.ec_cmd_queue.put(command)
    logging.debug('Commands now in queue: %d', self.ec_cmd_queue.qsize())
    # Add the EC UART as an output to be serviced.
    self.outputs.append(self.ec_uart_pty)

  def PackCommand(self, raw_cmd):
    r"""Packs a command for use with error checking.

    For error checking, we pack console commands in a particular format.  The
    format is as follows:

      &&[x][x][x][x]&{cmd}\n\n
      ^ ^    ^^    ^^  ^  ^-- 2 newlines.
      | |    ||    ||  |-- the raw console command.
      | |    ||    ||-- 1 ampersand.
      | |    ||____|--- 2 hex digits representing the CRC8 of cmd.
      | |____|-- 2 hex digits reprsenting the length of cmd.
      |-- 2 ampersands

    Args:
      raw_cmd: A pre-packed string which contains the raw command.

    Returns:
      A string which contains the packed command.
    """
    # Don't pack a single carriage return.
    if raw_cmd != '\r':
      # The command format is as follows.
      # &&[x][x][x][x]&{cmd}\n\n
      packed_cmd = []
      packed_cmd.append('&&')
      # The first pair of hex digits are the length of the command.
      packed_cmd.append('%02x' % len(raw_cmd))
      # Then the CRC8 of cmd.
      packed_cmd.append('%02x' % Crc8(raw_cmd))
      packed_cmd.append('&')
      # Now, the raw command followed by 2 newlines.
      packed_cmd.append(raw_cmd)
      packed_cmd.append('\n\n')
      return ''.join(packed_cmd)
    else:
      return raw_cmd

  def ProcessCommand(self, command):
    """Captures the input determines what actions to take.

    Args:
      command: A string representing the command sent by the user.
    """
    # Remove leading and trailing spaces only if this is an enhanced EC image.
    # For non-enhanced EC images, commands will be single characters at a time
    # and can be spaces.
    if self.enhanced_ec:
      command = command.strip(' ')

    # There's nothing to do if the command is empty.
    if len(command) == 0:
      return

    # Check for interrogation command.
    if command == EC_SYN:
      # User is requesting interrogation.  Send SYN as is.
      logging.debug('User requesting interrogation.')
      self.interrogating = True
      # Assume the EC isn't enhanced until we get a response.
      self.enhanced_ec = False
    elif self.enhanced_ec:
      # Enhanced EC images require the plaintext commands to be packed.
      command = self.PackCommand(command)
      # TODO(aaboagye): Make a dict of commands and keys and eventually,
      # handle partial matching based on unique prefixes.

    logging.debug('command: \'%s\'', command)
    self.EnqueueCmd(command)

  def HandleCmdRetries(self):
    """Attempts to retry commands if possible."""
    if self.cmd_retries > 0:
      # The EC encountered an error.  We'll have to retry again.
      logging.warning('Retrying command...')
      self.cmd_retries -= 1
      logging.warning('Retries remaining: %d', self.cmd_retries)
      # Retry the command and add the EC UART to the writers again.
      self.EnqueueCmd(self.last_cmd)
      self.outputs.append(self.ec_uart_pty)
    else:
      # We're out of retries, so just give up.
      logging.error('Command failed.  No retries left.')
      # Clear the command in progress.
      self.last_cmd = ''
      # Reset the retry count.
      self.cmd_retries = COMMAND_RETRIES

  def SendCmdToEC(self):
    """Sends a command to the EC."""
    # If we're retrying a command, just try to send it again.
    if self.cmd_retries < COMMAND_RETRIES:
      cmd = self.last_cmd
    else:
      # If we're not retrying, we should not be writing to the EC if we have no
      # items in our command queue.
      assert not self.ec_cmd_queue.empty()
      # Get the command to send.
      cmd = self.ec_cmd_queue.get()

    # Send the command.
    self.ec_uart_pty.write(cmd)
    self.ec_uart_pty.flush()
    logging.debug('Sent command to EC.')

    if self.enhanced_ec and cmd != EC_SYN:
      # Now, that we've sent the command, store the current command as the last
      # command sent.  If we encounter an error string, we will attempt to retry
      # this command.
      if cmd != self.last_cmd:
        self.last_cmd = cmd
        # Reset the retry count.
        self.cmd_retries = COMMAND_RETRIES
    # Remove the EC UART from the writers while we wait for a response.
    self.outputs.remove(self.ec_uart_pty)

  def HandleECData(self):
    """Handle any debug prints from the EC."""
    logging.debug('EC has data')
    # Read what the EC sent us.
    data = os.read(self.ec_uart_pty.fileno(), EC_MAX_READ)
    logging.debug('got: \'%s\'', binascii.hexlify(data))
    if '&E' in data and self.enhanced_ec:
      # We received an error, so we should retry it if possible.
      logging.warning('Error string found in data.')
      self.HandleCmdRetries()
      return

    # If we were interrogating, check the response and update our knowledge
    # of the current EC image.
    if self.interrogating:
      self.enhanced_ec = data == EC_ACK
      if self.enhanced_ec:
        logging.debug('The current EC image seems enhanced.')
      else:
        logging.debug('The current EC image does NOT seem enhanced.')
      # Done interrogating.
      self.interrogating = False
    # For now, just forward everything the EC sends us.
    logging.debug('Forwarding to user...')
    self.dbg_pipe.send(data)

  def HandleUserData(self):
    """Handle any incoming commands from the user."""
    logging.debug('Command data available.  Begin processing.')
    data = self.cmd_pipe.recv()
    # Process the command.
    self.ProcessCommand(data)


def Crc8(data):
  """Calculates the CRC8 of data.

  The generator polynomial used is: x^8 + x^2 + x + 1.
  This is the same implementation that is used in the EC.

  Args:
    data: A string of data that we wish to calculate the CRC8 on.

  Returns:
    crc >> 8: An integer representing the CRC8 value.
  """
  crc = 0
  for byte in data:
    crc ^= (ord(byte) << 8)
    for _ in range(8):
      if crc & 0x8000:
        crc ^= (0x1070 << 3)
      crc <<= 1
  return crc >> 8


def StartLoop(interp):
  """Starts an infinite loop of servicing the user and the EC.

  StartLoop checks to see if there are any commands to process, processing them
  if any, and forwards EC output to the user.

  When sending a command to the EC, we send the command once and check the
  response to see if the EC encountered an error when receiving the command.  An
  error condition is reported to the interpreter by a string with at least one
  '&' and 'E'.  The full string is actually '&&EE', however it's possible that
  the leading ampersand or trailing 'E' could be dropped.  If an error is
  encountered, the interpreter will retry up to the amount configured.

  Args:
    interp: An Interpreter object that has been properly initialised.
  """
  while True:
    readable, writeable, _ = select.select(interp.inputs, interp.outputs, [])

    for obj in readable:
      # Handle any debug prints from the EC.
      if obj is interp.ec_uart_pty:
        interp.HandleECData()

      # Handle any commands from the user.
      elif obj is interp.cmd_pipe:
        interp.HandleUserData()

    for obj in writeable:
      # Send a command to the EC.
      if obj is interp.ec_uart_pty:
        interp.SendCmdToEC()
