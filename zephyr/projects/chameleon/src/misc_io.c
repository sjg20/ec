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
#include <drivers/i2c.h>
#include <shell/shell.h>
#include <sys/__assert.h>
#include <sys/printk.h>

#include "gpio_signal.h"

DECLARE_GPIOS_FOR(misc);

/**
 * @brief Set up the misc I/O pins
 *
 * TP125 and TP126 are set up as outputs that could be connected to a
 * scope or logic analyzer for extra debugging help.
 * BOARD_VERSION[2:0] are set up as inputs.
 *
 * @param ptr Provided by the SYS_INIT API, unused here.
 */
static int misc_io_init(const struct device *ptr)
{
	ARG_UNUSED(ptr);

	int ret;

	ret = gpio_pin_configure(GPIO_LOOKUP(misc, tp126), GPIO_OUTPUT_LOW);
	if (ret < 0) {
		printk("gpio_pin_configure(tp126) failed, ret = %d\n", ret);
		return ret;
	}
	ret = gpio_pin_configure(GPIO_LOOKUP(misc, tp125), GPIO_OUTPUT_LOW);
	if (ret < 0) {
		printk("gpio_pin_configure(tp125) failed, ret = %d\n", ret);
		return ret;
	}
	ret = gpio_pin_configure(GPIO_LOOKUP(misc, board_version_2),
				 GPIO_INPUT);
	if (ret < 0) {
		printk("gpio_set(board_version_2) failed, ret = %d\n", ret);
		return ret;
	}
	ret = gpio_pin_configure(GPIO_LOOKUP(misc, board_version_1),
				 GPIO_INPUT);
	if (ret < 0) {
		printk("gpio_set(board_version_1) failed, ret = %d\n", ret);
		return ret;
	}
	ret = gpio_pin_configure(GPIO_LOOKUP(misc, board_version_0),
				 GPIO_INPUT);
	if (ret < 0) {
		printk("gpio_set(board_version_0) failed, ret = %d\n", ret);
		return ret;
	}

	return 0;
}
SYS_INIT(misc_io_init, APPLICATION, 75);

int misc_io_get_board_version(void)
{
	int version;
	int val;

	val = gpio_pin_get(GPIO_LOOKUP(misc, board_version_2));
	if (val < 0) {
		return val;
	}
	version = val << 2;

	val = gpio_pin_get(GPIO_LOOKUP(misc, board_version_1));
	if (val < 0) {
		return val;
	}
	version |= val << 1;

	val = gpio_pin_get(GPIO_LOOKUP(misc, board_version_0));
	if (val < 0) {
		return val;
	}
	version |= val;

	return version;
}

/* Shell commands for debugging/testing use. See README.md for details. */

/**
 * @brief Get an IO pin value
 *
 * @param shell Our shell data struct; needed for some shell APIs.
 * @param argc Count of arguments passed to this shell command. Unused.
 * @param argv Arguments for this shell command. argv[0] will be the name
 * of the IO to get.
 */
static int cmd_io_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int val;

	if (!strcmp(argv[0], "tp126")) {
		val = gpio_pin_get(GPIO_LOOKUP(misc, tp126));
	} else if (!strcmp(argv[0], "tp125")) {
		val = gpio_pin_get(GPIO_LOOKUP(misc, tp125));
	} else if (!strcmp(argv[0], "ver")) {
		val = misc_io_get_board_version();
	} else {
		/*
		 * The shell should not have called this function at all
		 * unless the subcommand matched a valid option.
		 */
		__ASSERT(false, "Shell called us with invalid subcommand.");
		return -EINVAL;
	}

	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, "%s = %d\n", argv[0],
		      val);
	return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(
	get_io, SHELL_CMD_ARG(tp126, NULL, "TP126", cmd_io_get, 1, 0),
	SHELL_CMD_ARG(tp125, NULL, "TP125", cmd_io_get, 1, 0),
	SHELL_CMD_ARG(ver, NULL, "BOARD_VERSION[2:0]", cmd_io_get, 1, 0),
	SHELL_SUBCMD_SET_END);

/**
 * @brief Set an IO pin value
 *
 * @param shell Our shell data struct; needed for some shell APIs.
 * @param argc Count of arguments passed to this shell command. Unused.
 * @param argv Arguments for this shell command. argv[0] will be the name
 * of the IO to set. argv[1] should be "on" or "off".
 */
static int cmd_io_set(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int ret;
	int val;

	if (!strcmp(argv[1], "on")) {
		val = 1;
	} else if (!strcmp(argv[1], "off")) {
		val = 0;
	} else {
		shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT,
			      "please indicate 'on' or 'off'\n");
		return -EINVAL;
	}

	if (!strcmp(argv[0], "tp126")) {
		ret = gpio_pin_set(GPIO_LOOKUP(misc, tp126), val);
	} else if (!strcmp(argv[0], "tp125")) {
		ret = gpio_pin_set(GPIO_LOOKUP(misc, tp125), val);
	} else {
		/* The shell should not have called this function at all
		 * unless the subcommand matched a valid option.
		 */
		__ASSERT(false, "Shell called us with invalid subcommand.");
		return -EINVAL;
	}

	return ret;
}
SHELL_STATIC_SUBCMD_SET_CREATE(
	set_io, SHELL_CMD_ARG(tp126, NULL, "TP126 [on|off]", cmd_io_set, 2, 0),
	SHELL_CMD_ARG(tp125, NULL, "TP125 [on|off]", cmd_io_set, 2, 0),
	SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(
	io_cmds, SHELL_CMD(get, &get_io, "get IO value", cmd_io_get),
	SHELL_CMD(set, &set_io, "get IO pin", cmd_io_get),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(io, &io_cmds, "Control IO pins", NULL);
