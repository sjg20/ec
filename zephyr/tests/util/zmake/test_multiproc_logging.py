# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import io
import logging
import os
import unittest.mock as mock
import threading

import zmake.multiproc


def test_read_output_from_pipe():
    barrier = threading.Barrier(2)
    pipe = os.pipe()
    fd = io.TextIOWrapper(os.fdopen(pipe[0], 'rb'), encoding='utf-8')
    logger = mock.Mock(spec=logging.Logger)
    logger.log.side_effect = lambda log_lvl, line: barrier.wait()
    zmake.multiproc.log_output(logger, logging.DEBUG, fd)
    os.write(pipe[1], 'Hello\n'.encode('utf-8'))
    barrier.wait()
    logger.log.assert_called_with(logging.DEBUG, 'Hello')


def test_read_output_from_second_pipe():
    """Test that we can read from more than one pipe.

    This is particularly important since we will block on a read/select once we
    have a file descriptor. It is important that we break from the select and
    start it again with the updated list when a new one is added.
    """
    barrier = threading.Barrier(2)
    pipes = [os.pipe(), os.pipe()]
    fds = [io.TextIOWrapper(os.fdopen(pipes[0][0], 'rb'), encoding='utf-8'),
           io.TextIOWrapper(os.fdopen(pipes[1][0], 'rb'), encoding='utf-8')]

    logger = mock.Mock(spec=logging.Logger)
    logger.log.side_effect = lambda log_lvl, line: barrier.wait()

    zmake.multiproc.log_output(logger, logging.DEBUG, fds[0])
    zmake.multiproc.log_output(logger, logging.ERROR, fds[1])

    os.write(pipes[1][1], 'Hello\n'.encode('utf-8'))
    barrier.wait()
    logger.log.assert_called_with(logging.ERROR, 'Hello')


def test_read_output_after_another_pipe_closed():
    """Test processing output from a pipe after closing another.

    Since we don't want to complicate the API. File descriptors are
    automatically pruned away when closed. Make sure that the other descriptors
    remain functional when that happens.
    """
    barrier = threading.Barrier(2)
    pipes = [os.pipe(), os.pipe()]
    fds = [io.TextIOWrapper(os.fdopen(pipes[0][0], 'rb'), encoding='utf-8'),
           io.TextIOWrapper(os.fdopen(pipes[1][0], 'rb'), encoding='utf-8')]

    logger = mock.Mock(spec=logging.Logger)
    logger.log.side_effect = lambda log_lvl, line: barrier.wait()

    zmake.multiproc.log_output(logger, logging.DEBUG, fds[0])
    zmake.multiproc.log_output(logger, logging.ERROR, fds[1])

    fds[0].close()
    os.write(pipes[1][1], 'Hello\n'.encode('utf-8'))
    barrier.wait()
    logger.log.assert_called_with(logging.ERROR, 'Hello')
