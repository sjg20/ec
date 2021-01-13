# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Common miscellaneous utility functions for zmake."""

import os
import pathlib
import re
import shlex


def locate_cros_checkout():
    """Find the path to the ChromiumOS checkout.

    Returns:
        The first directory found with a .repo directory in it,
        starting by checking the CROS_WORKON_SRCROOT environment
        variable, then scanning upwards from the current directory,
        and finally from a known set of common paths.
    """
    def propose_checkouts():
        yield os.getenv('CROS_WORKON_SRCROOT')

        path = pathlib.Path.cwd()
        while path.resolve() != pathlib.Path('/'):
            yield path
            path = path / '..'

        yield '/mnt/host/source'
        yield pathlib.Path.home() / 'trunk'
        yield pathlib.Path.home() / 'chromiumos'

    for path in propose_checkouts():
        if not path:
            continue
        path = pathlib.Path(path)
        if (path / '.repo').is_dir():
            return path.resolve()

    raise FileNotFoundError('Unable to locate a ChromiumOS checkout')


def locate_zephyr_base(checkout, version):
    """Locate the path to the Zephyr RTOS in a ChromiumOS checkout.

    Args:
        checkout: The path to the ChromiumOS checkout.
        version: The requested zephyr version, as a tuple of integers.

    Returns:
        The path to the Zephyr source.
    """
    return (checkout / 'src' / 'third_party' / 'zephyr' / 'main' /
            'v{}.{}'.format(*version[:2]))


def read_kconfig_file(path):
    """Parse a Kconfig file.

    Args:
        path: The path to open.

    Returns:
        A dictionary of kconfig items to their values.
    """
    result = {}
    with open(path) as f:
        for line in f:
            line, _, _ = line.partition('#')
            line = line.strip()
            if line:
                name, _, value = line.partition('=')
                result[name.strip()] = value.strip()
    return result


def write_kconfig_file(path, config, only_if_changed=True):
    """Write out a dictionary to Kconfig format.

    Args:
        path: The path to write to.
        config: The dictionary to write.
        only_if_changed: Set to True if the file should not be written
            unless it has changed.
    """
    if only_if_changed:
        if path.exists() and read_kconfig_file(path) == config:
            return
    with open(path, "w") as f:
        for name, value in config.items():
            f.write('{}={}\n'.format(name, value))


def parse_zephyr_version(version_string):
    """Parse a human-readable version string (e.g., "v2.4") as a tuple.

    Args:
        version_string: The human-readable version string.

    Returns:
        A 2-tuple or 3-tuple of integers representing the version.
    """
    match = re.fullmatch(r'v?(\d+)[._](\d+)(?:[._](\d+))?', version_string)
    if not match:
        raise ValueError(
            "{} does not look like a Zephyr version.".format(version_string))
    return tuple(int(x) for x in match.groups() if x is not None)


def repr_command(argv):
    """Represent an argument array as a string.

    Args:
        argv: The arguments of the command.

    Returns:
        A string which could be pasted into a shell for execution.
    """
    return ' '.join(shlex.quote(str(arg)) for arg in argv)


def update_symlink(target_path, link_path):
    """Create a symlink if it does not exist, or links to a different path.

    Args:
        target_path: A Path-like object of the desired symlink path.
        link_path: A Path-like object of the symlink.
    """
    target = target_path.resolve()
    if (not link_path.is_symlink()
            or pathlib.Path(os.readlink(link_path)).resolve() != target):
            if link_path.exists():
                link_path.unlink()
            link_path.symlink_to(target)
