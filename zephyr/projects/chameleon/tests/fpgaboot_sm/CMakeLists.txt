# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13.1)

project(fpgaboot_sm)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
FILE(GLOB app_sources *.c)
target_include_directories(app PRIVATE ../../include)
target_sources(app PRIVATE
	${app_sources}
	../../src/fpgaboot_sm.c)
