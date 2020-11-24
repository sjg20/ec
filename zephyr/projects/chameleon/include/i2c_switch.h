/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __I2C_SWITCH_H
#define __I2C_SWITCH_H

#include <zephyr.h>

enum i2c_switch_reset_value {
	RESET_ASSERTED = 0,
	RESET_DEASSERTED
};

/**
 * @brief Select which bus is enabled
 *
 * @param bus bus number, 0-3, of which bus to enable. Any number <0 or >3
 * will disable all buses.
 *
 * @retval 0 if successful.
 * @retval -ENODEV or error return from Zephyr API functions.
 */
int i2c_switch_select_bus(int bus);

/**
 * @brief Control the reset pin on the i2c switch.
 *
 * @param val one of enum i2c_switch_reset_value to assert or deassert the
 * reset pin
 *
 * @retval 0 if successful.
 * @retval -ENODEV or error return from Zephyr API functions.
 */
int i2c_switch_reset(enum i2c_switch_reset_value val);

/**
 * @brief Return interrupt status register
 *
 * The MAX7367 interrupt status indicates which of the downstream interrupt
 * lines are asserted. Reading the interrupt status does not clear it; the
 * downstream devices have to de-assert their interrupt lines.
 *
 * @note The only interrupt line connected to anything other than a pull-up
 * is `I2C1_EXP_INT2_L`, and only the EP91A6SQ chips are connected to it.
 *
 * @retval
 * -EIO General input / output error.
 * @retval
 * bitfield where bit 0 is set for INT0, bit 1 set for INT1, etc.
 */
int i2c_switch_get_int_status(void);

#endif
