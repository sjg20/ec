/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __LED_H
#define __LED_H

#include <zephyr.h>

/** @brief Turn the PG_CHAMELEON LED on or off.
 *
 * This function controls the LED defined as "power_good_led" in the
 * devicetree.
 *
 * @param turn_on True to turn it on, False to turn it off.
 *
 * @retval 0 if successful.
 * @retval -ENODEV or error return from gpio_pin_configure if error.
 */
int led_powergood(bool turn_on);

#endif
