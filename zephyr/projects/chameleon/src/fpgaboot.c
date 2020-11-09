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
#include "fpgaboot.h"
#include "fpgaboot_sm.h"

DECLARE_GPIOS_FOR(fpga);

/**
 * FPGA state
 */
static enum fpga_state state;

int fpgaboot_set_boot_mode(enum fpga_boot_mode mode)
{
	if (!fpgaboot_is_steady_state(state)) {
		return -EBUSY;
	}
	switch (mode) {
	case FPGA_BOOT_EMMC:
		gpio_pin_set(GPIO_LOOKUP(fpga, boot_mode1), 0);
		gpio_pin_set(GPIO_LOOKUP(fpga, boot_mode0), 0);
		break;
	case FPGA_BOOT_QSPI:
		gpio_pin_set(GPIO_LOOKUP(fpga, boot_mode1), 0);
		gpio_pin_set(GPIO_LOOKUP(fpga, boot_mode0), 1);
		break;
	case FPGA_BOOT_SDIO:
		gpio_pin_set(GPIO_LOOKUP(fpga, boot_mode1), 1);
		gpio_pin_set(GPIO_LOOKUP(fpga, boot_mode0), 1);
		break;
	default:
		__ASSERT(false, "Unsupported fpga_boot_mode value");
		return -EINVAL;
	}

	return 0;
}

K_MSGQ_DEFINE(fpgaboot_msgq, sizeof(enum fpga_event), 10, 1);

int fpgaboot_power_on(void)
{
	enum fpga_event event = FPGABOOT_POWER_ON_REQ;

	return k_msgq_put(&fpgaboot_msgq, &event, K_NO_WAIT);
}

int fpgaboot_power_off(void)
{
	enum fpga_event event = FPGABOOT_POWER_OFF_REQ;

	return k_msgq_put(&fpgaboot_msgq, &event, K_NO_WAIT);
}

static struct gpio_callback pwr_good_cb_data;

static void pwr_good_cb(const struct device *dev, struct gpio_callback *cb,
			uint32_t pins)
{
	int val = gpio_pin_get(GPIO_LOOKUP(fpga, pwr_good));
	enum fpga_event event = val ? FPGABOOT_SOM_PWR_GOOD_ASSERTED :
					    FPGABOOT_SOM_PWR_GOOD_DEASSERTED;

	k_msgq_put(&fpgaboot_msgq, &event, K_NO_WAIT);
}

static struct gpio_callback fpga_done_cb_data;

static void fpga_done_cb(const struct device *dev, struct gpio_callback *cb,
			 uint32_t pins)
{
	int val = gpio_pin_get(GPIO_LOOKUP(fpga, fpga_done));
	enum fpga_event event = val ? FPGABOOT_SOM_FPGA_DONE_ASSERTED :
					    FPGABOOT_SOM_FPGA_DONE_DEASSERTED;

	k_msgq_put(&fpgaboot_msgq, &event, K_NO_WAIT);
}

/*
 * @brief Post a TIMER_EXPIRED event when the time expires
 *
 * @param dummy Unused, required by signature
 */
static void fpgaboot_timeout(struct k_timer *dummy)
{
	ARG_UNUSED(dummy);

	enum fpga_event event = FPGABOOT_TIMER_EXPIRED;

	k_msgq_put(&fpgaboot_msgq, &event, K_NO_WAIT);
}

K_TIMER_DEFINE(fpgaboot_timer, fpgaboot_timeout, NULL);
/*
 * Values for waiting for FPGA events
 *
 * Both values are wild guesses unless/until characterized.
 * The SHORT_TIMER is to wait from PWR_EN to PWR_GOOD.
 * The LONG_TIME is to wait from releasing reset to seeing DONE.
 */
#define SHORT_TIMER K_MSEC(500)
#define LONG_TIMER K_SECONDS(10)

/**
 * @brief Execute the actions specified by the state machine
 *
 * The FPGA power/boot state machine returns a bitfield with possible
 * actions that need to be taken. This function does the actions.
 * The actions are defined in fpgaboot_sm.h starting at `FPGABOOT_ACTION_NONE`.
 *
 * @param actions Bitfield of actions to be taken
 */
static void fpgaboot_handle_actions(uint32_t actions)
{
	if (actions & FPGABOOT_ASSERT_SOM_PWR_EN) {
		__ASSERT(
			(actions & FPGABOOT_DEASSERT_SOM_PWR_EN) == 0,
			"Don't assert and de-assert SOM_PWR_EN at the same time!");
		gpio_pin_set(GPIO_LOOKUP(fpga, pwr_en), 1);
	}
	if (actions & FPGABOOT_DEASSERT_SOM_PWR_EN) {
		__ASSERT(
			(actions & FPGABOOT_ASSERT_SOM_PWR_EN) == 0,
			"Don't assert and de-assert SOM_PWR_EN at the same time!");
		gpio_pin_set(GPIO_LOOKUP(fpga, pwr_en), 0);
	}
	/* NB: SOM_POR_L_LOAD_L is active low, so "assert" == "set to 0V" */
	if (actions & FPGABOOT_ASSERT_SOM_POR_L_LOAD_L) {
		__ASSERT(
			(actions & FPGABOOT_DEASSERT_SOM_POR_L_LOAD_L) == 0,
			"Don't assert and de-assert SOM_POR_L_LOAD_l at the same time!");
		gpio_pin_set(GPIO_LOOKUP(fpga, por_l_load_l), 0);
	}
	if (actions & FPGABOOT_DEASSERT_SOM_POR_L_LOAD_L) {
		__ASSERT(
			(actions & FPGABOOT_ASSERT_SOM_POR_L_LOAD_L) == 0,
			"Don't assert and de-assert SOM_POR_L_LOAD_l at the same time!");
		gpio_pin_set(GPIO_LOOKUP(fpga, por_l_load_l), 1);
	}
	if (actions & FPGABOOT_START_SHORT_TIMER) {
		k_timer_start(&fpgaboot_timer, SHORT_TIMER, K_NO_WAIT);
	}
	if (actions & FPGABOOT_START_LONG_TIMER) {
		k_timer_start(&fpgaboot_timer, LONG_TIMER, K_NO_WAIT);
	}
	if (actions & FPGABOOT_STOP_TIMER) {
		k_timer_stop(&fpgaboot_timer);
	}
}

/**
 * Define priotity for the fpgaboot task
 */
#define FPGABOOT_PRIORITY 10

/**
 * Define the size of the stack for the fpgaboot task.
 */
#define FPGABOOT_STACK_SIZE 500

/**
 * Zephyr macro to allocate the stack area.
 */
K_THREAD_STACK_DEFINE(fpgaboot_stack_area, FPGABOOT_STACK_SIZE);

/**
 * Internal data structure for tracking the fpgaboot task.
 */
static struct k_thread fpgaboot_thread_data;

/**
 * @brief Run the FPGA power/boot state machine
 *
 * This task controls the FPGA power/boot sequence. The task listens on a
 * message queue for events: requests from callers and messages from
 * interrupt handlers for GPIO inputs. When it gets an event from the queue,
 * it passes that to the state machine, and then takes the output `actions`
 * and does the things that the bitfields indicate should be done.

 * @param unused1 Required by the Zephyr thread API. Unused.
 * @param unused2 Required by the Zephyr thread API. Unused.
 * @param unused3 Required by the Zephyr thread API. Unused.
 */
static void fpgaboot_task(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	enum fpga_event event;
	uint32_t actions;

	k_thread_name_set(NULL, "fpgaboot_task");

	fpgaboot_init_state_machine(&state, &actions);
	fpgaboot_handle_actions(actions);
	for (;;) {
		int ret = k_msgq_get(&fpgaboot_msgq, &event, K_FOREVER);

		if (ret == 0) {
			fpgaboot_run_state_machine(event, &state, &actions);
			fpgaboot_handle_actions(actions);
		}
	}
}

/**
 * @brief Initialize the FPGA GPIOs and start the state machine task
 *
 * @note Also sets the FPGA boot mode to SDIO.
 *
 * @param ptr Unused argument, required by SYS_INIT
 * @retval 0
 * No error
 * @retval Non-zero
 * Error code
 */
static int fpgaboot_init(const struct device *ptr)
{
	ARG_UNUSED(ptr);

	int ret;

	ret = gpio_pin_configure(GPIO_LOOKUP(fpga, pwr_en), GPIO_OUTPUT);
	if (ret < 0) {
		printk("gpio_pin_configure(pwr_en) failed, ret = %d\n", ret);
		return ret;
	}

	ret = gpio_pin_configure(GPIO_LOOKUP(fpga, boot_mode1), GPIO_OUTPUT);
	if (ret < 0) {
		printk("gpio_pin_configure(boot_mode1) failed, ret = %d\n",
		       ret);
		return ret;
	}

	ret = gpio_pin_configure(GPIO_LOOKUP(fpga, boot_mode0), GPIO_OUTPUT);
	if (ret < 0) {
		printk("gpio_pin_configure(boot_mode0) failed, ret = %d\n",
		       ret);
		return ret;
	}
	/*
	 * With the BOOT_MODE[1:0] GPIOs configured, now set the default boot
	 * mode to boot from the microSD card slot on the Chameleon v3 board.
	 */
	fpgaboot_set_boot_mode(FPGA_BOOT_SDIO);

	ret = gpio_pin_configure(GPIO_LOOKUP(fpga, por_l_load_l), GPIO_OUTPUT);
	if (ret < 0) {
		printk("gpio_pin_configure(por_l_load_l) failed, ret = %d\n",
		       ret);
		return ret;
	}

	/* Configure pwr_good as input, pull-up, interrupt on change. */
	ret = gpio_pin_configure(GPIO_LOOKUP(fpga, pwr_good),
				 GPIO_INPUT | GPIO_PULL_UP);
	if (ret < 0) {
		printk("gpio_set(pwr_good) failed, ret = %d\n", ret);
		return ret;
	}
	ret = gpio_pin_interrupt_configure(GPIO_LOOKUP(fpga, pwr_good),
					   GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on PWR_GOOD\n",
		       ret);
		return ret;
	}
	gpio_init_callback(&pwr_good_cb_data, pwr_good_cb,
			   BIT(gpio_pins[GPIO_ENUM(DT_PATH(fpga, pwr_good))]));
	gpio_add_callback(gpio_devs[GPIO_ENUM(DT_PATH(fpga, pwr_good))],
			  &pwr_good_cb_data);

	/* Configure fpga_done as input, pull-up, interrupt on change. */
	ret = gpio_pin_configure(GPIO_LOOKUP(fpga, fpga_done),
				 GPIO_INPUT | GPIO_PULL_UP);
	if (ret < 0) {
		printk("gpio_set(fpga_done) failed, ret = %d\n", ret);
		return ret;
	}
	ret = gpio_pin_interrupt_configure(GPIO_LOOKUP(fpga, fpga_done),
					   GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on FPGA_DONE\n",
		       ret);
		return ret;
	}
	gpio_init_callback(&fpga_done_cb_data, fpga_done_cb,
			   BIT(gpio_pins[GPIO_ENUM(DT_PATH(fpga, fpga_done))]));
	gpio_add_callback(gpio_devs[GPIO_ENUM(DT_PATH(fpga, fpga_done))],
			  &fpga_done_cb_data);

	/* Create the fpgaboot task and start it. */
	k_thread_create(&fpgaboot_thread_data, fpgaboot_stack_area,
			K_THREAD_STACK_SIZEOF(fpgaboot_stack_area),
			fpgaboot_task, NULL, NULL, NULL, FPGABOOT_PRIORITY, 0,
			K_NO_WAIT);

	return 0;
}
SYS_INIT(fpgaboot_init, APPLICATION, 75);

/**
 * @brief Display the FPGA status
 *
 * @param shell Our shell data struct; needed for some shell APIs.
 * @param argc Count of arguments passed to this shell command. Unused.
 * @param argv Arguments for this shell command. Unused.
 */
static int cmd_status(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT,
		      "FPGA steady status: %s\n",
		      fpgaboot_is_steady_state(state) ? "true" : "false");
	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, "BOOT_MODE = %d%d\n",
		      gpio_pin_get(GPIO_LOOKUP(fpga, boot_mode1)),
		      gpio_pin_get(GPIO_LOOKUP(fpga, boot_mode0)));
	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, "PWR_EN = %d\n",
		      gpio_pin_get(GPIO_LOOKUP(fpga, pwr_en)));
	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, "PWR_GOOD = %d\n",
		      gpio_pin_get(GPIO_LOOKUP(fpga, pwr_good)));
	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, "POR_L_LOAD_L = %d\n",
		      gpio_pin_get(GPIO_LOOKUP(fpga, por_l_load_l)));
	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, "FPGA_DONE = %d\n",
		      gpio_pin_get(GPIO_LOOKUP(fpga, fpga_done)));
	return 0;
}

/**
 * @brief Turn on the FPGA
 *
 * @param shell Our shell data struct; needed for some shell APIs.
 * @param argc Count of arguments passed to this shell command. Unused.
 * @param argv Arguments for this shell command. Unused.
 */
static int cmd_on(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, "FPGA power on ...\n");
	fpgaboot_power_on();
	return 0;
}

/**
 * @brief Turn off the FPGA
 *
 * @param shell Our shell data struct; needed for some shell APIs.
 * @param argc Count of arguments passed to this shell command. Unused.
 * @param argv Arguments for this shell command. Unused.
 */
static int cmd_off(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, "FPGA power off ...\n");
	fpgaboot_power_off();
	return 0;
}
/**
 * @brief Set the FPGA boot mode
 *
 * @param shell Our shell data struct; needed for some shell APIs.
 * @param argc Count of arguments passed to this shell command. Unused.
 * @param argv Arguments for this shell command. Unused.
 */
static int cmd_boot(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret;

	if (strcmp(argv[0], "emmc") == 0) {
		ret = fpgaboot_set_boot_mode(FPGA_BOOT_EMMC);
	} else if (strcmp(argv[0], "qspi") == 0) {
		ret = fpgaboot_set_boot_mode(FPGA_BOOT_QSPI);
	} else if (strcmp(argv[0], "sdio") == 0) {
		ret = fpgaboot_set_boot_mode(FPGA_BOOT_SDIO);
	} else {
		/* The shell should not have called this function at all
		 * unless the subcommand matched a valid option.
		 */
		__ASSERT(false, "Shell called us with invalid subcommand.");
		return -EINVAL;
	}
	if (ret == 0) {
		shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT,
			      "FPGA boot mode set to %s\n", argv[0]);
	} else if (ret == -EBUSY) {
		shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT,
			      "FPGA is busy. Try again in a few seconds.\n");
	} else {
		shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT,
			      "Unknown error %d.\n", ret);
	}
	return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(
	boot_mode,
	SHELL_CMD_ARG(emmc, NULL, "set FPGA to boot from eMMC", cmd_boot, 1, 0),
	SHELL_CMD_ARG(qspi, NULL, "set FPGA to boot from QSPI", cmd_boot, 1, 0),
	SHELL_CMD_ARG(sdio, NULL, "set FPGA to boot from SDIO", cmd_boot, 1, 0),
	SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(
	fpga_cmds, SHELL_CMD(status, NULL, "get status", cmd_status),
	SHELL_CMD(on, NULL, "power on and boot", cmd_on),
	SHELL_CMD(off, NULL, "power off", cmd_off),
	SHELL_CMD(bootmode, &boot_mode, "set FPGA boot mode", NULL),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(fpga, &fpga_cmds, "FPGAcommands", NULL);
