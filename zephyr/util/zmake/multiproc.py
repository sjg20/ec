# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os
import select
import threading

"""Zmake multiprocessing utility module.

This module is used to aid in zmake's multiprocessing. It contains tools
available to log output from multiple processes on the fly. This means that a
process does not need to finish before the output is available to the developer
on the screen.
"""

# A local pipe use to signal the look that a new file descriptor was added and
# should be included in the select statement.
_logging_interrupt_pipe = os.pipe()
# A condition variable used to synchronize logging operations.
_logging_cv = threading.Condition()
# A map of file descriptors to their logger/logging level tuple.
_logging_map = {}


def _log_fd(fd):
    """Log information from a single file descriptor.

    This function is BLOCKING. It will read from the given file descriptor until
    either the end of line is read or EOF. Once EOF is read it will remove the
    file descriptor from _logging_map so it will no longer be used.
    Additionally, in some cases, the file descriptor will be closed (caused by
    a call to Popen.wait()). In these cases, the file descriptor will also be
    removed from the map as it is no longer valid.
    """
    with _logging_cv:
        logger, log_level = _logging_map[fd]
        if fd.closed:
            del _logging_map[fd]
            return
        line = fd.readline()
        if not line:
            # EOF
            del _logging_map[fd]
            return
        line = line.strip()
        if line:
            logger.log(log_level, line)


def _prune_logging_fds():
    """Prune the current file descriptors under _logging_map.

    This function will iterate over the logging map and check for closed file
    descriptors. Every closed file descriptor will be removed.
    """
    with _logging_cv:
        remove = [fd for fd in _logging_map.keys() if fd.closed]
        for fd in remove:
            del _logging_map[fd]


def _logging_loop():
    """The primary logging thread loop.

    This is the entry point of the logging thread. It will listen for (1) any
    new data on the output file descriptors that were added via log_output and
    (2) any new file descriptors being added by log_output. Once a file
    descriptor is ready to be read, this function will call _log_fd to perform
    the actual read and logging.
    """
    while True:
        with _logging_cv:
            _logging_cv.wait_for(lambda: _logging_map)
            keys = list(_logging_map.keys()) + [_logging_interrupt_pipe[0]]
        try:
            fds, _, _ = select.select(keys, [], [])
        except ValueError:
            # One of the file descriptors must be closed, prune them and try
            # again.
            _prune_logging_fds()
            continue
        if _logging_interrupt_pipe[0] in fds:
            # We got a dummy byte sent by log_output, this is a signal used to
            # break out of the blocking select.select call to tell us that the
            # file descriptor set has changed. We just need to read the byte and
            # remove this descriptor from the list. If we actually have data
            # that should be read it will be read in the for loop below.
            os.read(_logging_interrupt_pipe[0], 1)
            fds.remove(_logging_interrupt_pipe[0])
        for fd in fds:
            _log_fd(fd)


_logging_thread = threading.Thread(target=_logging_loop, daemon=True)


def log_output(logger, log_level, file_descriptor):
    """Log the output from the given file descriptor.

    Args:
        logger: The logger object to use.
        log_level: The logging level to use.
        file_descriptor: The file descriptor to read from.
    """
    with _logging_cv:
        if not _logging_thread.is_alive():
            _logging_thread.start()
        _logging_map[file_descriptor] = (logger, log_level)
        # Write a dummy byte to the pipe to break the select so we can add the
        # new fd.
        os.write(_logging_interrupt_pipe[1], b'x')
        # Notify the condition so we can run the select on the current fds.
        _logging_cv.notify_all()
