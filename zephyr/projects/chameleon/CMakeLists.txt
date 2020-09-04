# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13.1)
set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(BOARD chameleon)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(chameleon)

target_sources(app PRIVATE src/main.c)
