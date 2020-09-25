/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/adc.h>
#include <drivers/gpio.h>
#include <shell/shell.h>
#include <sys/printk.h>
#include "sysmon.h"

#define SEL_GPIO	DT_GPIO_LABEL(DT_PATH(zephyr_user), sysmon_sel_gpios)
#define SEL_PIN		DT_GPIO_PIN(DT_PATH(zephyr_user), sysmon_sel_gpios)

enum sysmon_sel {
	SYSMON_SEL_SOM,		/**< Select the FPGA modules voltage rails. */
	SYSMON_SEL_REGULATORS	/**< Select the regulator voltage rails. */
};

struct adc_hdl {
	char *device_name;
	const struct device *dev;
	struct adc_channel_cfg channel_config;
};

/** @brief The STM32 ADC resolution is fixed at 12 bits. */
#define STM_ADC_RESOLUTION 12

/** @brief Declare the ADCs this module uses and the configuration.
 *
 * Note that this module does not use ADC2.
 * 1. All of the analog signals are connected to ADC3 inputs, or inputs that
 *    either ADC1 or ADC2 can use.
 * 2. ADC1 and ADC2 share an ISR. When both are enabled, Zephyr trips an
 *    assertion.
 */
static struct adc_hdl adc_list[] = {
	{
		.device_name = DT_LABEL(DT_NODELABEL(adc1)),
		.dev = NULL,
		.channel_config = {
			.gain = ADC_GAIN_1,
			.reference = ADC_REF_INTERNAL,
			.acquisition_time = ADC_ACQ_TIME_DEFAULT,
			.channel_id = 15,
		},
	},
	{
		.device_name = DT_LABEL(DT_NODELABEL(adc3)),
		.dev = NULL,
		.channel_config = {
			.gain = ADC_GAIN_1,
			.reference = ADC_REF_INTERNAL,
			.acquisition_time = ADC_ACQ_TIME_DEFAULT,
			.channel_id = 4,
		},
	}

};

/** @brief Select which set of voltages SYSMON can read
 *
 * There are more voltages to monitor than ADC inputs, so the hardware
 * uses analog switches for the SYSMON1:8 inputs. When SYSMON_SEL is
 * high, the analog switches are set to connect the voltage regulator
 * (or voltage divider) outputs to the SYSMON inputs. When SYSMON_SEL
 * is low, the analog switches connect voltage outputs from the FPGA
 * module to the SYSMON inputs.
 *
 * @param sel SYSMON_SEL_REGULATORS to select the voltage regulator/divider,
 * or SYSMON_SEL_SOM to select the FPGA SOM voltages.
 *
 * @retval 0 if successful.
 * @retval error return from gpio_pin_configure if error.
 */
static int sysmon_sel(enum sysmon_sel sel)
{
	int ret;
	int gpio_dir = (sel == SYSMON_SEL_REGULATORS) ?
		GPIO_OUTPUT_HIGH : GPIO_OUTPUT_LOW;
	const struct device *gpio_dev = device_get_binding(SEL_GPIO);

	if (gpio_dev == NULL) {
		printk("Cannot get device SEL_GPIO\n");
		return -ENODEV;
	}

	ret = gpio_pin_configure(gpio_dev, SEL_PIN, gpio_dir);
	if (ret < 0) {
		printk("gpio_pin_configure(SEL_PIN) failed, ret = %d\n", ret);
		return ret;
	}

	return 0;
}

int sysmon_init(void)
{
	int ret = 0;

	/* Set the analog switches for the regulator outputs. */
	ret = sysmon_sel(SYSMON_SEL_REGULATORS);
	if (ret < 0)
		return ret;

	/* Configure the ADCs. */
	for (int idx = 0 ; idx < ARRAY_SIZE(adc_list) ; ++idx) {
		adc_list[idx].dev = device_get_binding(
			adc_list[idx].device_name);
		if (adc_list[idx].dev == NULL) {
			printk("Cannot get device %s\n",
				adc_list[idx].device_name);
			return -ENODEV;
		}

		ret = adc_channel_setup(adc_list[idx].dev,
			&adc_list[idx].channel_config);
		if (ret < 0) {
			printk("adc_channel_setup failed, ret = %d\n", ret);
			return ret;
		}
	}

	return ret;
}

/* Temporary function that reads all of the ADCs and prints the raw values.
 * If this code looks bad to you, that's OK; it's supposed to look bad.
 * The code is only a quick-and-dirty proof that we can read the ADCs and
 * get back values that are sensible. It's not supposed to be pretty or
 * even maintainable. The next revision (done under b/169365982) will
 * remove this code entirely and replace it with actually clean code. But
 * for the purposes of checkpointing progress, this code exists.
 */
static int cmd_read_adcs(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret;
	uint16_t sample_buffer;

	for (int chan = 10 ; chan <= 15 ; ++chan) {
		const struct adc_sequence sequence = {
			.channels	= BIT(chan),
			.buffer		= &sample_buffer,
			.buffer_size	= sizeof(sample_buffer),
			.resolution	= STM_ADC_RESOLUTION,
		};

		ret = adc_read(adc_list[0].dev, &sequence);
		if (ret < 0) {
			printk("adc_read failed, ret = %d\n", ret);
			return ret;
		}
		printk("%u\n", sample_buffer);
	}

	for (int chan = 4 ; chan <= 8 ; ++chan) {
		const struct adc_sequence sequence = {
			.channels	= BIT(chan),
			.buffer		= &sample_buffer,
			.buffer_size	= sizeof(sample_buffer),
			.resolution	= STM_ADC_RESOLUTION,
		};

		ret = adc_read(adc_list[1].dev, &sequence);
		if (ret < 0) {
			printk("adc_read failed, ret = %d\n", ret);
			return ret;
		}
		printk("%u\n", sample_buffer);
	}

	return 0;
}
SHELL_CMD_REGISTER(read_adcs, NULL, "read the ADCs and print", cmd_read_adcs);
