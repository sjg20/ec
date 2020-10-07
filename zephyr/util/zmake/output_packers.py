# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Types which provide many builds and composite them into a single binary."""

import zmake.build_config as build_config


class BasePacker:
    """Abstract base for all packers."""
    def __init__(self, project):
        self.project = project

    def configs(self):
        """Get all of the build configurations necessary.

        Yields:
            2-tuples of config name and a BuildConfig.
        """
        yield 'singleimage', build_config.BuildConfig()

    def pack_firmware(self, work_dir, jobclient):
        """Pack a firmware image.

        Config names from the configs generator are passed as keyword
        arguments, with each argument being set to the path of the
        build directory.

        Args:
            work_dir: A directory to write outputs and temporary files
            into.
            jobclient: A JobClient object to use.

        Yields:
            2-tuples of the path of each file in the work_dir (or any
            other directory) which should be copied into the output
            directory, and the output filename.
        """
        raise NotImplementedError('Abstract method not implemented')


class ElfPacker(BasePacker):
    """Raw proxy for ELF output of a single build."""
    def pack_firmware(self, work_dir, jobclient, singleimage):
        yield singleimage / 'zephyr' / 'zephyr.elf', 'zephyr.elf'


class RawBinPacker(BasePacker):
    """Raw proxy for BIN output of a single build."""
    def pack_firmware(self, work_dir, jobclient, singleimage):
        yield singleimage / 'zephyr' / 'zephyr.bin', 'zephyr.bin'


# A dictionary mapping packer config names to classes.
packer_registry = {
    'elf': ElfPacker,
    'raw': RawBinPacker,
}
