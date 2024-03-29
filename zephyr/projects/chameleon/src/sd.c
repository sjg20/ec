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
#include <sys/__assert.h>
#include <sys/printk.h>
#include "gpio_signal.h"
#include "sd.h"

DECLARE_GPIOS_FOR(sd_mux);

enum sd_state mux_state; /**< Currently selected state */

int sd_mux_set(enum sd_state state)
{
	/* To enable the FPGA to access the SD card:
	 *	SD_MUX_EN_L := 0
	 *	SD_MUX_SEL := 0
	 *	USD_PWR_EN := 1
	 *	USD_PWR_SEL := 1
	 * To enable the USB port to access the SD card:
	 *	SD_MUX_EN_L := 0
	 *	SD_MUX_SEL := 1
	 *	USD_PWR_EN := 1
	 *	USD_PWR_SEL := 0
	 * To disconnect the SD card from either:
	 *	SD_MUX_EN_L := 1
	 *	SD_MUX_SEL := don't care
	 *	USD_PWR_EN := 0
	 *	USD_PWR_SEL := don't care
	 */

	int ret;
	int mux_en = (state == SD_MUX_OFF) ? 1 : 0;
	int mux_sel = (state == SD_MUX_USB) ? 1 : 0;
	int pwr_en = (state != SD_MUX_OFF) ? 1 : 0;
	int pwr_sel = (state == SD_MUX_FPGA) ? 1 : 0;

	mux_state = state;

	ret = gpio_pin_set(GPIO_LOOKUP(sd_mux, sd_mux_sel), mux_sel);
	if (ret < 0) {
		printk("gpio_pin_set(sd_mux_sel) failed, ret = %d\n", ret);
		return ret;
	}

	ret = gpio_pin_set(GPIO_LOOKUP(sd_mux, sd_mux_en_l), mux_en);
	if (ret < 0) {
		printk("gpio_pin_set(sd_mux_en+l) failed, ret = %d\n", ret);
		return ret;
	}

	ret = gpio_pin_set(GPIO_LOOKUP(sd_mux, usd_pwr_sel), pwr_sel);
	if (ret < 0) {
		printk("gpio_pin_set(usd_pwr_sel) failed, ret = %d\n", ret);
		return ret;
	}

	ret = gpio_pin_set(GPIO_LOOKUP(sd_mux, usd_pwr_en), pwr_en);
	if (ret < 0) {
		printk("gpio_pin_set(usd_pwr_en) failed, ret = %d\n", ret);
		return ret;
	}

	return 0;
}

enum sd_state sd_mux_get(void)
{
	return mux_state;
}

int sd_get_cd_det(void)
{
	return gpio_pin_get(GPIO_LOOKUP(sd_mux, usd_cd_det));
}

/**
 * @brief Initialize the SD mux GPIOs to a disconnected state
 *
 * The SD mux controls whether the SD card signals are connected, and
 * if they are connected, whether to the FPGA or the USB controller.
 * Set up the GPIOs and then set the mux to disconnected.
 */
static int sd_mux_init(const struct device *ptr)
{
	ARG_UNUSED(ptr);

	return sd_mux_set(SD_MUX_OFF);
}
SYS_INIT(sd_mux_init, APPLICATION, 90);

/**
 * @brief Handle the "sd off" shell command.
 */
static int cmd_sd_off(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return sd_mux_set(SD_MUX_OFF);
}

/**
 * @brief Handle the "sd fpga" shell command.
 */
static int cmd_sd_fpga(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return sd_mux_set(SD_MUX_FPGA);
}

/**
 * @brief Handle the "sd usb" shell command.
 */
static int cmd_sd_usb(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return sd_mux_set(SD_MUX_USB);
}

/**
 * @brief Handle the "sd status" shell command.
 */
static int cmd_sd_status(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("The SD mux is set to ");
	switch (sd_mux_get()) {
	case SD_MUX_OFF:
		printk("disconnected\n");
		break;
	case SD_MUX_FPGA:
		printk("FPGA\n");
		break;
	case SD_MUX_USB:
		printk("USB\n");
		break;
	default:
		printk("invalid value, %d\n", (int)mux_state);
	}

	printk("CD_DET = %d\n", sd_get_cd_det());

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sd_cmds, SHELL_CMD(off, NULL, "disconnect the SD mux", cmd_sd_off),
	SHELL_CMD(fpga, NULL, "connect the SD mux to the FPGA", cmd_sd_fpga),
	SHELL_CMD(usb, NULL, "connect the SD mux to the USB", cmd_sd_usb),
	SHELL_CMD(status, NULL, "get SD mux status", cmd_sd_status),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(sd, &sd_cmds, "SD mux commands", NULL);
