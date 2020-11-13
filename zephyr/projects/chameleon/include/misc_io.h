/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __MISC_IO_H
#define __MISC_IO_H

#include <zephyr.h>

/**
 * @brief Get the board version number.
 *
 * Three I/Os on an I2C GPIO expander are tied to power or ground to indicate
 * the hardware version number of the board.
 *
 * @retval >=0 board version number
 * @retval <0 error number returned by `gpio_pin_get`
 */
int misc_io_get_board_version(void);

#endif
