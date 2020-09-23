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
#include <shell/shell.h>
#include <sys/printk.h>
#include "led.h"

#define PG_NODE DT_NODELABEL(power_good_led)
#define PG_LED	DT_GPIO_LABEL(PG_NODE, gpios)
#define PG_PIN	DT_GPIO_PIN(PG_NODE, gpios)

int led_powergood(bool turn_on)
{
	struct device *dev;
	int ret;

	dev = device_get_binding(PG_LED);	/* "GPIOB" also works */
	if (dev == NULL)
		return -ENODEV;

	if (turn_on)
		ret = gpio_pin_configure(dev, PG_PIN, GPIO_OUTPUT_HIGH);
	else
		ret = gpio_pin_configure(dev, PG_PIN, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		printk("gpio_pin_configure failed, ret = %d\n", ret);
		return ret;
	}

	return 0;
}

static int cmd_led(const struct shell *shell, size_t argc, char **argv)
{
	/* The shell should never call a command without a valid argv[0]. */
	__ASSERT(argc >= 1, "Shell passed invalid argument count");
	if (argc == 0)
		return -EINVAL;

	if (strcmp(argv[0], "on") == 0)
		led_powergood(true);
	else if (strcmp(argv[0], "off") == 0)
		led_powergood(false);
	else {
		/* The shell should not have called this function at all
		 * unless the subcommand matched a valid option.
		 */
		__ASSERT(false, "Shell called us with invalid subcommand.");
		return -EINVAL;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(led_cmds,
	SHELL_CMD(on, NULL, "turn LED on", cmd_led),
	SHELL_CMD(off, NULL, "turn LED off", cmd_led),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(led, &led_cmds, "LED commands", NULL);
