# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(BOARD scarlet)

# Set the compiler needed for this board
# TODO(b/161461212): move absolute path into another file
set(CROSS_COMPILE /opt/coreboot-sdk/bin/arm-eabi-)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(scarlet)

target_sources(app PRIVATE src/main.c)
