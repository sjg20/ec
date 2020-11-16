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
#include "misc_io.h"

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
 * @brief Get value of TP126
 */
static int cmd_io_get_tp126(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int val = gpio_pin_get(GPIO_LOOKUP(misc, tp126));

	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, "TP126 = %d\n", val);
	return 0;
}

/**
 * @brief Get value of TP125
 */
static int cmd_io_get_tp125(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int val = gpio_pin_get(GPIO_LOOKUP(misc, tp125));

	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, "TP125 = %d\n", val);
	return 0;
}

/**
 * @brief Get the board version from BOARD_VERSION[2:0]
 */
static int cmd_io_get_ver(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int val = misc_io_get_board_version();

	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT,
		      "BOARD_VERSION[2:0] = %d\n", val);
	return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(
	get_io, SHELL_CMD(tp126, NULL, "TP126", cmd_io_get_tp126),
	SHELL_CMD(tp125, NULL, "TP125", cmd_io_get_tp125),
	SHELL_CMD(ver, NULL, "BOARD_VERSION[2:0]", cmd_io_get_ver),
	SHELL_SUBCMD_SET_END);

/**
 * @brief Set TP126 to on
 */
static int cmd_io_set_tp126_on(const struct shell *shell, size_t argc,
			       char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return gpio_pin_set(GPIO_LOOKUP(misc, tp126), 1);
}

/**
 * @brief Set TP126 to off
 */
static int cmd_io_set_tp126_off(const struct shell *shell, size_t argc,
				char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return gpio_pin_set(GPIO_LOOKUP(misc, tp126), 0);
}

/**
 * @brief Set TP125 to on
 */
static int cmd_io_set_tp125_on(const struct shell *shell, size_t argc,
			       char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return gpio_pin_set(GPIO_LOOKUP(misc, tp125), 1);
}

/**
 * @brief Set TP126 to off
 */
static int cmd_io_set_tp125_off(const struct shell *shell, size_t argc,
				char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return gpio_pin_set(GPIO_LOOKUP(misc, tp125), 0);
}
SHELL_STATIC_SUBCMD_SET_CREATE(
	tp126_on_off, SHELL_CMD(on, NULL, "set TP126 on", cmd_io_set_tp126_on),
	SHELL_CMD(off, NULL, "set TP126 off", cmd_io_set_tp126_off),
	SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(
	tp125_on_off, SHELL_CMD(on, NULL, "set TP125 on", cmd_io_set_tp125_on),
	SHELL_CMD(off, NULL, "set TP125 off", cmd_io_set_tp125_off),
	SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(
	set_io, SHELL_CMD(tp126, &tp126_on_off, "set TP126", NULL),
	SHELL_CMD(tp125, &tp125_on_off, "set TP125", NULL),
	SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(io_cmds,
			       SHELL_CMD(get, &get_io, "get IO value", NULL),
			       SHELL_CMD(set, &set_io, "set IO pin", NULL),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(io, &io_cmds, "Control IO pins", NULL);
