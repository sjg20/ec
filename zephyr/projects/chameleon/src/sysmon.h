/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __SYSMON_H
#define __SYSMON_H

#include <zephyr.h>

/** @brief Initialize the system monitor
 *
 * sysmon is responsible for reading several ADC channels to monitor
 * the supply current and voltages. There are more voltages to monitor
 * than ADC input pins, so there are are analog switches to select which
 * sets of voltages is connected to the ADC inputs. sysmon needs to
 * initialize a GPIO to control the analog switches in addition to
 * intializing the ADCs to sample inputs.
 *
 * @retval 0 if successful.
 * @retval -ENODEV or error return from gpio_pin_configure if error.
 */
int sysmon_init(void);

#endif
