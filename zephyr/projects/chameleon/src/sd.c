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
#include "sd.h"

/**
 * This structure holds the GPIO name, pin, and device pointer for a pin
 * that is used in the SD card control.
 */
struct sd_ctrl_pin {
	const char *name;	/**< Name of the GPIO device */
	gpio_pin_t pin;		/**< Pin number */
	const struct device *dev;	/**< Pointer from device_get_binding */
};

/**
 * This structure holds the sd_ctrl_pin structs for every pin that is part
 * of the SD card control (mux, power, and detect), plus the last state
 * of the mux that we were requested to set.
 */
struct sd_ctrl_data {
	struct sd_ctrl_pin mux_sel;	/**< Select SD card connections */
	struct sd_ctrl_pin mux_en_l;	/**< Enable the mux */
	struct sd_ctrl_pin usd_pwr_sel; /**< Select power from USB or 3.3V */
	struct sd_ctrl_pin usd_pwr_en;	/**< Enable power */
	struct sd_ctrl_pin usd_cd_det;	/**< Card detect input */

	enum sd_state state;	/**< Currently selected state */
};

static struct sd_ctrl_data ctrl;

/**
 * @brief Initialize an sd_ctrl_pin struct with a device and pin
 *
 * @param p_mux_pin The struct to be initialized
 * @param name The GPIO device name, from DT_ macros
 * @param pin The pin number
 *
 * @retval 0 if successful, negative errno.h constant if error
 */
static int sd_mux_pin_init(struct sd_ctrl_pin *p_mux_pin, const char *name,
	gpio_pin_t pin)
{
	__ASSERT_NO_MSG(p_mux_pin != NULL);
	__ASSERT_NO_MSG(name != NULL);

	p_mux_pin->name = name;
	p_mux_pin->pin = pin;
	p_mux_pin->dev = device_get_binding(p_mux_pin->name);
	if (p_mux_pin->dev == NULL) {
		return -ENODEV;
	}

	return 0;
}

/**
 * @brief Configure the pin that an sd_ctrl_pin struct points to
 *
 * This function is a helper that just pulls the device and pin from
 * the sd_ctrl_pin struct.
 *
 * @param p_mux_pin The struct to be initialized
 * @param flags The GPIO flags to apply to the pin
 *
 * @retval 0 if successful, negative errno.h constant if error
 */
static int sd_mux_pin_config(struct sd_ctrl_pin *p_mux_pin, gpio_flags_t flags)
{
	__ASSERT_NO_MSG(p_mux_pin != NULL);

	return gpio_pin_configure(p_mux_pin->dev, p_mux_pin->pin, flags);
}

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
	int mux_en = (state == SD_MUX_OFF) ?
		GPIO_OUTPUT_HIGH : GPIO_OUTPUT_LOW;
	int mux_sel = (state == SD_MUX_USB) ?
		GPIO_OUTPUT_HIGH : GPIO_OUTPUT_LOW;
	int pwr_en = (state != SD_MUX_OFF) ?
		GPIO_OUTPUT_HIGH : GPIO_OUTPUT_LOW;
	int pwr_sel = (state == SD_MUX_FPGA) ?
		GPIO_OUTPUT_HIGH : GPIO_OUTPUT_LOW;

	ctrl.state = state;

	ret = sd_mux_pin_config(&ctrl.mux_sel, mux_sel);
	if (ret < 0) {
		printk("gpio_pin_configure(SD_MUX_SEL_PIN) failed, ret = %d\n",
			ret);
		return ret;
	}

	ret = sd_mux_pin_config(&ctrl.mux_en_l, mux_en);
	if (ret < 0) {
		printk("gpio_pin_configure(SD_MUX_EN_L_PIN) failed, ret = %d\n",
			ret);
		return ret;
	}

	ret = sd_mux_pin_config(&ctrl.usd_pwr_sel, pwr_sel);
	if (ret < 0) {
		printk("gpio_pin_configure(USD_PWR_SEL_PIN) failed, ret = %d\n",
			ret);
		return ret;
	}

	ret = sd_mux_pin_config(&ctrl.usd_pwr_en, pwr_en);
	if (ret < 0) {
		printk("gpio_pin_configure(USD_PWR_EN_PIN) failed, ret = %d\n",
			ret);
		return ret;
	}

	return 0;
}

enum sd_state sd_mux_get(void)
{
	return ctrl.state;
}

int sd_get_cd_det(void)
{
	return gpio_pin_get(ctrl.usd_cd_det.dev, ctrl.usd_cd_det.pin);
}

#define SD_MUX_SEL_GPIO DT_GPIO_LABEL(DT_PATH(zephyr_user), sd_mux_sel_gpios)
#define SD_MUX_SEL_PIN DT_GPIO_PIN(DT_PATH(zephyr_user), sd_mux_sel_gpios)

#define SD_MUX_EN_L_GPIO DT_GPIO_LABEL(DT_PATH(zephyr_user), sd_mux_en_l_gpios)
#define SD_MUX_EN_L_PIN DT_GPIO_PIN(DT_PATH(zephyr_user), sd_mux_en_l_gpios)

#define USD_PWR_SEL_GPIO DT_GPIO_LABEL(DT_PATH(zephyr_user), usd_pwr_sel_gpios)
#define USD_PWR_SEL_PIN DT_GPIO_PIN(DT_PATH(zephyr_user), usd_pwr_sel_gpios)

#define USD_PWR_EN_GPIO DT_GPIO_LABEL(DT_PATH(zephyr_user), usd_pwr_en_gpios)
#define USD_PWR_EN_PIN DT_GPIO_PIN(DT_PATH(zephyr_user), usd_pwr_en_gpios)

#define USD_CD_DET_GPIO DT_GPIO_LABEL(DT_PATH(zephyr_user), usd_cd_det_gpios)
#define USD_CD_DET_PIN DT_GPIO_PIN(DT_PATH(zephyr_user), usd_cd_det_gpios)

int sd_mux_init(void)
{
	int ret;

	ret = sd_mux_pin_init(&ctrl.mux_en_l, SD_MUX_EN_L_GPIO,
		SD_MUX_EN_L_PIN);
	if (ret != 0) {
		printk("Setup failed for SD_MUX_EN_L_GPIO, ret = %d\n", ret);
		return ret;
	}

	ret = sd_mux_pin_init(&ctrl.mux_sel, SD_MUX_SEL_GPIO, SD_MUX_SEL_PIN);
	if (ret != 0) {
		printk("Setup failed for SD_MUX_SEL_GPIO, ret = %d\n", ret);
		return ret;
	}

	ret = sd_mux_pin_init(&ctrl.usd_pwr_sel, USD_PWR_SEL_GPIO,
		USD_PWR_SEL_PIN);
	if (ret != 0) {
		printk("Setup failed for USD_PWR_SEL_GPIO, ret = %d\n", ret);
		return ret;
	}

	ret = sd_mux_pin_init(&ctrl.usd_pwr_en, USD_PWR_EN_GPIO,
		USD_PWR_EN_PIN);
	if (ret != 0) {
		printk("Setup failed for USD_PWR_EN_GPIO, ret = %d\n", ret);
		return ret;
	}

	ret = sd_mux_pin_init(&ctrl.usd_cd_det, USD_CD_DET_GPIO,
		USD_CD_DET_PIN);
	if (ret != 0) {
		printk("Setup failed for USD_CD_DET_GPIO, ret = %d\n", ret);
		return ret;
	}
	ret = sd_mux_pin_config(&ctrl.usd_cd_det, GPIO_INPUT | GPIO_PULL_UP);
	if (ret < 0) {
		printk("gpio_pin_configure(USD_CD_DET_PIN) failed, ret = %d\n",
			ret);
		return ret;
	}

	return sd_mux_set(SD_MUX_OFF);
}

/**
 * @brief Handle the "sd off/fpga/usb" shell commands.
 */
static int cmd_sd(const struct shell *shell, size_t argc, char **argv)
{
	/* The shell should never call a command without a valid argv[0]. */
	__ASSERT(argc >= 1, "Shell passed invalid argument count");
	if (argc == 0) {
		return -EINVAL;
	}

	if (strcmp(argv[0], "off") == 0) {
		return sd_mux_set(SD_MUX_OFF);
	} else if (strcmp(argv[0], "fpga") == 0) {
		return sd_mux_set(SD_MUX_FPGA);
	} else if (strcmp(argv[0], "usb") == 0) {
		return sd_mux_set(SD_MUX_USB);
	}

	/*
	 * The shell should not have called this function at all
	 * unless the subcommand matched a valid option.
	 */
	__ASSERT(false, "Shell called us with invalid subcommand.");
	return -EINVAL;
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
		printk("invalid value, %d\n", (int)ctrl.state);
	}

	printk("CD_DET = %d\n", sd_get_cd_det());

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sd_cmds,
	SHELL_CMD(off, NULL, "disconnect the SD mux", cmd_sd),
	SHELL_CMD(fpga, NULL, "connect the SD mux to the FPGA", cmd_sd),
	SHELL_CMD(usb, NULL, "connect the SD mux to the USB", cmd_sd),
	SHELL_CMD(status, NULL, "get SD mux status", cmd_sd_status),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(sd, &sd_cmds, "SD mux commands", NULL);
