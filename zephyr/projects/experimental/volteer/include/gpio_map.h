/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __ZEPHYR_GPIO_MAP_H
#define __ZEPHYR_GPIO_MAP_H

#include <devicetree.h>
#include <gpio_signal.h>

/*
 * Without https://github.com/zephyrproject-rtos/zephyr/pull/29282, we need
 * to manually link GPIO_ defines that platform/ec code expects to the
 * enum gpio_signal values that are generated by device tree bindings.
 *
 * Note we only need to create aliases for GPIOs that are referenced in common
 * platform/ec code.
 */
#define GPIO_EN_PP3300_A NAMED_GPIO(en_pp3300_a)
#define GPIO_EN_PP5000_A NAMED_GPIO(en_pp5000_a)


#endif /* __ZEPHYR_GPIO_MAP_H */
