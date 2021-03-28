/* Copyright 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* ADC interface for Chrome EC */

#ifndef __CROS_EC_ADC_H
#define __CROS_EC_ADC_H

#include "common.h"

#define ADC_READ_ERROR -1  /* Value returned by adc_read_channel() on error */

#ifdef CONFIG_ZEPHYR
#ifdef CONFIG_PLATFORM_EC_ADC

#define ZSHIM_ADC_ENUM(val)	DT_CAT(ADC_, val)
#define ZSHIM_ADC_TYPE(id)	\
	ZSHIM_ADC_ENUM(DT_ENUM_UPPER_TOKEN(id, enum_name))
#define ADC_TYPE_WITH_COMMA(id)	ZSHIM_ADC_TYPE(id),

enum adc_channel {
#if DT_NODE_EXISTS(DT_PATH(named_adc_channels))
	DT_FOREACH_CHILD(DT_INST(0, named_adc_channels), ADC_TYPE_WITH_COMMA)
#endif
	ADC_CH_COUNT
};
#undef ADC_TYPE_WITH_COMMA

struct adc_t {
	const char *name;
	uint8_t input_ch;
	int factor_mul;
	int factor_div;
};

extern const struct adc_t adc_channels[];
#endif /* CONFIG_PLATFORM_EC_ADC */
#endif /* CONFIG_ZEPHYR */

/*
 * Boards which use the ADC interface must provide enum adc_channel in the
 * board.h file.  See chip/$CHIP/adc_chip.h for additional chip-specific
 * requirements.
 */

/**
 * Read an ADC channel.
 *
 * @param ch		Channel to read
 *
 * @return The scaled ADC value, or ADC_READ_ERROR if error.
 */
int adc_read_channel(enum adc_channel ch);

/**
 * Enable ADC watchdog. Note that interrupts might come in repeatedly very
 * quickly when ADC output goes out of the accepted range.
 *
 * @param ain_id	The AIN to be watched by the watchdog.
 * @param high		The high threshold that the watchdog would trigger
 *			an interrupt when exceeded.
 * @param low		The low threshold.
 *
 * @return		EC_SUCCESS, or non-zero if any error.
 */
int adc_enable_watchdog(int ain_id, int high, int low);

/**
 * Disable ADC watchdog.
 *
 * @return		EC_SUCCESS, or non-zero if any error.
 */
int adc_disable_watchdog(void);

/**
 * Set the delay between ADC watchdog samples. This can be used as a trade-off
 * of power consumption and performance.
 *
 * @param delay_ms      The delay in milliseconds between two ADC watchdog
 *                      samples.
 *
 * @return              EC_SUCCESS, or non-zero if any error or not supported.
 */
int adc_set_watchdog_delay(int delay_ms);

#endif  /* __CROS_EC_ADC_H */
