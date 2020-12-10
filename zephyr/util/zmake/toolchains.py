# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Definitions of toolchain variables."""

import zmake.build_config as build_config


# Mapping of toolchain names -> (λ (module-paths) build-config)
toolchains = {
    'coreboot-sdk': lambda modules: build_config.BuildConfig(
        cmake_defs={'TOOLCHAIN_ROOT': str(modules['zephyr-chrome']),
                    'ZEPHYR_TOOLCHAIN_VARIANT': 'coreboot-sdk'}),
    'arm-none-eabi': lambda _: build_config.BuildConfig(
        cmake_defs={'ZEPHYR_TOOLCHAIN_VARIANT': 'cross-compile',
                    'CROSS_COMPILE': '/usr/bin/arm-none-eabi-'}),
}


def get_toolchain(name, module_paths):
    """Get a toolchain by name.

    Args:
        name: The name of the toolchain.
        module_paths: Dictionary mapping module names to paths.

    Returns:
        The corresponding BuildConfig from the defined toolchains, if
        one exists, otherwise a simple BuildConfig which sets
        ZEPHYR_TOOLCHAIN_VARIANT to the corresponding name.
    """
    if name in toolchains:
        return toolchains[name](module_paths)
    return build_config.BuildConfig(
        cmake_defs={'ZEPHYR_TOOLCHAIN_VARIANT': name})
