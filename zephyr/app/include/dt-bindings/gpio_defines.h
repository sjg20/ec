/*
 * Copyright 2020 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef DT_BINDINGS_GPIO_DEFINES_H_
#define DT_BINDINGS_GPIO_DEFINES_H_

/*
 * The GPIO_INPUT and GPIO_OUTPUT defines are normally not available to
 * the device tree. For GPIOs that are controlled by the platform/ec module, we
 * allow device tree to set the initial state.
 *
 * Note the defines in this file are copies from <drivers/gpio.h>
 */

/** Enables pin as input. */
#define GPIO_INPUT              (1U << 8)

/** Enables pin as output, no change to the output state. */
#define GPIO_OUTPUT             (1U << 9)

/* Initializes output to a low state. */
#define GPIO_OUTPUT_INIT_LOW    (1U << 10)

/* Initializes output to a high state. */
#define GPIO_OUTPUT_INIT_HIGH   (1U << 11)

/** Configures GPIO pin as output and initializes it to a low state. */
#define GPIO_OUTPUT_LOW         (GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW)

/** Configures GPIO pin as output and initializes it to a high state. */
#define GPIO_OUTPUT_HIGH        (GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH)

#endif /* DT_BINDINGS_GPIO_DEFINES_H_ */
