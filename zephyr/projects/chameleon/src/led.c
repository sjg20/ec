/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <sys/printk.h>
#include "led.h"

#define PG_NODE DT_NODELABEL(power_good_led)
#define PG_LED	DT_GPIO_LABEL(PG_NODE, gpios)
#define PG_PIN	DT_GPIO_PIN(PG_NODE, gpios)

int led_powergood_on(void)
{
	struct device *dev;
	int ret;

	dev = device_get_binding(PG_LED);	/* "GPIOB" also works */
	if (dev == NULL)
		return -ENODEV;

	ret = gpio_pin_configure(dev, PG_PIN, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("gpio_pin_configure failed, ret = %d\n", ret);
		return ret;
	}

	return 0;
}
