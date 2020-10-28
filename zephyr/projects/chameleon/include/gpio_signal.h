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

#define NULL_WITH_COMMA(path) NULL,

#define GPIO_DEV_NAME_WITH_COMMA(path) DT_GPIO_LABEL(path, gpios),

#define GPIO_ENUM(path) DT_CAT(GPIO_, path)
#define GPIO_ENUM_WITH_COMMA(path) GPIO_ENUM(path),

#define DECLARE_GPIOS_FOR(path)                                            \
	static const gpio_pin_t gpio_pins[] = { DT_FOREACH_CHILD(          \
		DT_PATH(path), GPIO_PIN_WITH_COMMA) };                     \
	static const struct device *gpio_devs[] = { DT_FOREACH_CHILD(      \
		DT_PATH(path), NULL_WITH_COMMA) };                         \
	static const char *const gpio_dev_names[] = { DT_FOREACH_CHILD(    \
		DT_PATH(path), GPIO_DEV_NAME_WITH_COMMA) };                \
	enum { DT_FOREACH_CHILD(DT_PATH(path), GPIO_ENUM_WITH_COMMA) };    \
	static int local_init_devices_for_##path(const struct device *ptr) \
	{                                                                  \
		ARG_UNUSED(ptr);                                           \
		for (int idx = 0; idx < ARRAY_SIZE(gpio_devs); ++idx) {    \
			gpio_devs[idx] =                                   \
				device_get_binding(gpio_dev_names[idx]);   \
		}                                                          \
		return 0;                                                  \
	}                                                                  \
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
 * Assign variables of type `const struct gpio_config*`.
 *
 *  const struct gpio_config *sd_mux_sel = GPIO_LOOKUP(sd_mux, sd_mux_sel);
 *
 * Use the `dev` and `pin` members of that struct to call the Zephyr API.
 *
 *  gpio_pin_configure(sd_mux_sel->dev, sd_mux_sel->pin, flags);
 *  gpio_pin_set(sd_mux_sel->dev, sd_mux_sel->pin, value);
 *  gpio_pin_get(sd_mux_sel->dev, sd_mux_sel->pin);
 */

#endif
