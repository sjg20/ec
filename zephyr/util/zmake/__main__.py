# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The entry point into zmake."""
import argparse
import inspect
import pathlib
import sys

import zmake.zmake as zm
import zmake.util as util


def call_with_namespace(func, namespace):
    """Call a function with arguments applied from a Namespace.

    Args:
        func: The callable to call.
        namespace: The namespace to apply to the callable.

    Returns:
        The result of calling the callable.
    """
    kwds = {}
    sig = inspect.signature(func)
    names = [p.name for p in sig.parameters.values()]
    for name, value in vars(namespace).items():
        pyname = name.replace('-', '_')
        if pyname in names:
            kwds[pyname] = value
    return func(**kwds)


def main(argv=None):
    """The main function.

    Args:
        argv: Optionally, the command-line to parse, not including argv[0].

    Returns:
        Zero upon success, or non-zero upon failure.
    """
    if argv is None:
        argv = sys.argv[1:]

    parser = argparse.ArgumentParser()
    parser.add_argument('--checkout', type=pathlib.Path,
                        help='Path to ChromiumOS checkout')
    parser.add_argument('-j', '--jobs', type=int,
                        help='Degree of multiprogramming to use')
    sub = parser.add_subparsers(dest='subcommand', help='Subcommand')
    sub.required = True

    configure = sub.add_parser('configure')
    configure.add_argument(
        '--ignore-unsupported-zephyr-version', action='store_true',
        help="Don't warn about using an unsupported Zephyr version")
    configure.add_argument('-v', '--version', type=util.parse_zephyr_version,
                           help='Zephyr RTOS version')
    configure.add_argument('-t', '--toolchain', help='Name of toolchain to use')
    configure.add_argument('--zephyr-base', type=pathlib.Path,
                           help='Path to Zephyr source')
    configure.add_argument('-B', '--build-dir', type=pathlib.Path,
                           required=True, help='Build directory')
    configure.add_argument('-b', '--build', action='store_true',
                           dest='build_after_configure',
                           help='Run the build after configuration')
    configure.add_argument('project_dir', type=pathlib.Path,
                           help='Path to the project to build')

    build = sub.add_parser('build')
    build.add_argument('build_dir', type=pathlib.Path,
                       help='The build directory used during configuration')

    opts = parser.parse_args(argv)

    zmake = call_with_namespace(zm.Zmake, opts)
    subcommand_method = getattr(zmake, opts.subcommand.replace('-', '_'))
    call_with_namespace(subcommand_method, opts)
    return 0


if __name__ == '__main__':
    sys.exit(main())
