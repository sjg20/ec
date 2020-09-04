# SPDX-License-Identifier: BSD-3-Clause

board_runner_args(jlink "--device=STM32F103ZG" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
