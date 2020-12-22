/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <stdlib.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <shell/shell.h>
#include <sys/__assert.h>
#include <sys/printk.h>
#include "gpio_signal.h"
#include "i2c_switch.h"

DECLARE_GPIOS_FOR(i2c_switch);

#define MAX7367_ADDR 0x73
#define NUM_BUS 4

const struct device *i2c_dev;

/**
 * @brief Mutex for changing the I2C switch state.
 */
static K_MUTEX_DEFINE(i2c_switch_mutex);

int i2c_switch_select_bus(int bus)
{
	uint8_t ctrl_val = 0;

	/*
	 * Only values between 0 and NUM_BUS-1 can set a bit in the control
	 * register. All others we just write 0 to disable all buses.
	 */
	if (bus >= 0 && bus < NUM_BUS) {
		ctrl_val = 1 << bus;
	}

	/*
	 * If we are setting a valid value, lock a mutex so that other tasks
	 * can't override us. If we are setting the result to 0 (no secondary
	 * bus selected), that's equivalent to saying we're done with that I2C
	 * bus, and so unlock the mutex for another task to claim it.
	 */
	if (ctrl_val != 0) {
		k_mutex_lock(&i2c_switch_mutex, K_FOREVER);
	} else {
		k_mutex_unlock(&i2c_switch_mutex);
	}

	return i2c_write(i2c_dev, &ctrl_val, 1, MAX7367_ADDR);
}

int i2c_switch_reset(enum i2c_switch_reset_value val)
{
	int ret;

	ret = gpio_pin_set(GPIO_LOOKUP(i2c_switch, exp_reset), val);
	if (ret < 0) {
		printk("gpio_pin_set(exp_reset) failed, ret = %d\n", ret);
	}
	return ret;
}

int i2c_switch_get_int_status(void)
{
	uint8_t ctrl_val = 0;
	int ret;

	ret = i2c_read(i2c_dev, &ctrl_val, 1, MAX7367_ADDR);
	if (ret < 0) {
		return ret;
	}
	/* Shift right by 4 bits to put the interrupt bits into 0:3 */
	ctrl_val >>= 4;
	return ctrl_val;
}

/**
 * Binary semaphore to let the I2C expander IRQ handler wake a user task
 *
 * The I2C expander IRQ will signal this semaphone, and a userspace task
 * will wait on it. When the semaphore is signaled, the userspace task can
 * read the interrupt registers and determine what to do.
 */
K_SEM_DEFINE(exp_irq_sem, 0, 1);

static struct gpio_callback exp_irq_cb_data;

static void exp_irq_cb(const struct device *dev, struct gpio_callback *cb,
		       uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	k_sem_give(&exp_irq_sem);
}

/**
 * @brief Initialize the MAX7367 i2c switch
 */
static int init_i2c_switch(const struct device *ptr)
{
	ARG_UNUSED(ptr);

	int ret;

	i2c_dev = device_get_binding("I2C_1");

	if (!i2c_dev) {
		printk("%s: I2C_1 device not found.\n", __func__);
		return -ENODEV;
	}

	i2c_switch_reset(RESET_DEASSERTED);
	i2c_switch_select_bus(-1);

	ret = gpio_pin_interrupt_configure(GPIO_LOOKUP(i2c_switch, exp_irq),
					   GPIO_INT_EDGE_FALLING);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on exp_irq\n",
		       ret);
		return ret;
	}
	gpio_init_callback(
		&exp_irq_cb_data, exp_irq_cb,
		BIT(gpio_pins[GPIO_ENUM(DT_PATH(i2c_switch, exp_irq))]));
	gpio_add_callback(gpio_devs[GPIO_ENUM(DT_PATH(i2c_switch, exp_irq))],
			  &exp_irq_cb_data);

	return 0;
}
SYS_INIT(init_i2c_switch, APPLICATION, 10);

/**
 * @brief Handle the "i2c_switch" shell command.
 */
static int cmd_i2c_switch(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* The value to write to the switch will be in argv[1]. */
	int val = atoi(argv[1]);

	return i2c_switch_select_bus(val);
}
SHELL_CMD_ARG_REGISTER(i2c_switch, NULL, "set the I2C switch value",
		       cmd_i2c_switch, 2, 0);
