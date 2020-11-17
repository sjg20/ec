#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Build and test all of the Zephyr boards.

This is the entry point for the custom firmware builder workflow rZephyripe.
"""

import argparse
import multiprocessing
import os
import shutil
import subprocess
import sys


def build(opts):
    """Builds all Zephyr firmware targets"""
    temp_build_dir = os.path.join('/tmp', 'zbuild')
    targets = [
        'projects/experimental/volteer', 'projects/experimental/posix-ec'
    ]
    for target in targets:
        if os.path.exists(temp_build_dir):
            shutil.rmtree(temp_build_dir)

        print('Building {}'.format(target))
        rv = subprocess.run(
            ['zmake', 'configure', '-b', '-B', temp_build_dir, target],
            cwd=os.path.dirname(__file__)).returncode
        if rv != 0:
            return rv
    return 0


def test(opts):
    """Runs all of the unit tests for Zephyr firmware"""
    temp_build_dir = os.path.join('/tmp', 'zbuild')
    targets = [
        'tests/app/ec',
        '../ec/zephyr/test/base32',
        '../ec/zephyr/test/crc',
        '../ec/zephyr/test/hooks',
        '../ec/zephyr/test/tasks'
    ]
    for target in targets:
        if os.path.exists(temp_build_dir):
            shutil.rmtree(temp_build_dir)

        # Build the test
        print('\n\nTesting {}'.format(target))
        rv = subprocess.run(
            ['zmake', 'configure', '--test', '-B', temp_build_dir, target],
            cwd=os.path.dirname(__file__)).returncode

        if rv != 0:
            return rv
    return 0


def main(args):
    """Builds and tests all of the Zephyr targets and reports build metrics"""
    opts = parse_args(args)

    if not hasattr(opts, 'func'):
        print("Must select a valid sub command!")
        return -1

    # Run selectd sub command function
    return opts.func(opts)


def parse_args(args):
    parser = argparse.ArgumentParser(description=__doc__)

    # Would make this required=True, but not available until 3.7
    sub_cmds = parser.add_subparsers()

    build_cmd = sub_cmds.add_parser('build',
                                    help='Builds all firmware targets')
    build_cmd.set_defaults(func=build)

    test_cmd = sub_cmds.add_parser('test', help='Runs all firmware unit tests')
    test_cmd.set_defaults(func=test)

    return parser.parse_args(args)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
