/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __GPIO_SIGNAL_H
#define __GPIO_SIGNAL_H

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <init.h>
#include <drivers/gpio.h>

#define GPIO_PIN_WITH_COMMA(path) DT_GPIO_PIN(path, gpios),

#define GPIO_FLAGS_WITH_COMMA(path) DT_GPIO_FLAGS(path, gpios),

#define NULL_WITH_COMMA(path) NULL,

#define GPIO_DEV_NAME_WITH_COMMA(path) DT_GPIO_LABEL(path, gpios),

#define GPIO_ENUM(path) DT_CAT(GPIO_, path)
#define GPIO_ENUM_WITH_COMMA(path) GPIO_ENUM(path),

#define DECLARE_GPIOS_FOR(path)                                               \
	static const gpio_pin_t gpio_pins[] = { DT_FOREACH_CHILD(             \
		DT_PATH(path), GPIO_PIN_WITH_COMMA) };                        \
	static const int gpio_flags[] = { DT_FOREACH_CHILD(                   \
		DT_PATH(path), GPIO_FLAGS_WITH_COMMA) };                      \
	static const struct device *gpio_devs[] = { DT_FOREACH_CHILD(         \
		DT_PATH(path), NULL_WITH_COMMA) };                            \
	static const char *const gpio_dev_names[] = { DT_FOREACH_CHILD(       \
		DT_PATH(path), GPIO_DEV_NAME_WITH_COMMA) };                   \
	enum { DT_FOREACH_CHILD(DT_PATH(path), GPIO_ENUM_WITH_COMMA) };       \
	static int local_init_devices_for_##path(const struct device *ptr)    \
	{                                                                     \
		ARG_UNUSED(ptr);                                              \
		for (int idx = 0; idx < ARRAY_SIZE(gpio_devs); ++idx) {       \
			gpio_devs[idx] =                                      \
				device_get_binding(gpio_dev_names[idx]);      \
			if (!gpio_devs[idx]) {                                \
				printk("local_init_devices_for_%s: "          \
				       "device_get_binding(%s) failed!\n",    \
				       #path, gpio_dev_names[idx]);           \
				continue;                                     \
			}                                                     \
			int ret = gpio_pin_configure(gpio_devs[idx],          \
						     gpio_pins[idx],          \
						     gpio_flags[idx]);        \
			if (ret != 0) {                                       \
				printk("local_init_devices_for_%s: "          \
				       "gpio_pin_configure(%s, %d, %d) "      \
				       "failed, ret = %d\n",                  \
				       #path, gpio_dev_names[idx],            \
				       gpio_pins[idx], gpio_flags[idx], ret); \
				continue;                                     \
			}                                                     \
		}                                                             \
		return 0;                                                     \
	}                                                                     \
	SYS_INIT(local_init_devices_for_##path, APPLICATION, 1);

#define GPIO_LOOKUP_COMMON(path) \
	gpio_devs[GPIO_ENUM(path)], gpio_pins[GPIO_ENUM(path)]
#define GPIO_LOOKUP(...) GPIO_LOOKUP_COMMON(DT_PATH(__VA_ARGS__))

/*
 * In your C file, include this header and declare the GPIOs based on the
 * devicetree node.
 *
 *  #include "gpio_signal.h"
 *  DECLARE_GPIOS_FOR(sd_mux);
 *
 * This macro declares some static const arrays and a local function (called
 * from SYS_INIT) that will initialize all of the GPIOs.
 *
 * Use `GPIO_LOOKUP(sd_mux, sd_mux_sel)` anywhere you need a `struct device*`
 * and a `gpio_pin_t` together:
 *
 *  gpio_pin_set(GPIO_LOOKUP(sd_mux, sd_mux_sel), value);
 *  gpio_pin_get(GPIO_LOOKUP(sd_mux, sd_mux_sel));
 *
 * Use `GPIO_ENUM(DT_PATH(...))` if you need the `struct device*` or the
 * `gpio_pin_t` separately, such as for setting up interrupt handlers:
 *
 *  gpio_init_callback(&pwr_good_cb_data, pwr_good_cb,
 *                     BIT(gpio_pins[GPIO_ENUM(DT_PATH(fpga, pwr_good))]));
 *  gpio_add_callback(gpio_devs[GPIO_ENUM(DT_PATH(fpga, pwr_good))],
 *                    &pwr_good_cb_data);
 *
 */

#endif
