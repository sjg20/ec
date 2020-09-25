/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <zephyr.h>
#include <sys/printk.h>

#include "led.h"
#include "sysmon.h"

void main(void)
{
	int ret;

	printk("Hello World!\n");

	ret = led_powergood(true);
	if (ret < 0)
		printk("Failed to turn on POWERGOOD, ret = %d.\n", ret);
	else
		printk("POWERGOOD!\n");

	ret = sysmon_init();
	if (ret < 0)
		printk("sysmon_init failed, ret = %d.\n", ret);
	else
		printk("sysmon_init done\n");
}
