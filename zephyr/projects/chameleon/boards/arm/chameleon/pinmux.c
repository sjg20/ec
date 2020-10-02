/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

static const struct pin_config pinconf[] = {
	/* UART */
	{ STM32_PIN_PA9, STM32F1_PINMUX_FUNC_PA9_USART1_TX },
	{ STM32_PIN_PA10, STM32F1_PINMUX_FUNC_PA10_USART1_RX },

	/* ADC3 inputs */
	{ STM32_PIN_PF6, STM32F1_PINMUX_FUNC_PF6_ADC3_IN4 },
	{ STM32_PIN_PF7, STM32F1_PINMUX_FUNC_PF7_ADC3_IN5 },
	{ STM32_PIN_PF8, STM32F1_PINMUX_FUNC_PF8_ADC3_IN6 },
	{ STM32_PIN_PF9, STM32F1_PINMUX_FUNC_PF9_ADC3_IN7 },
	{ STM32_PIN_PF10, STM32F1_PINMUX_FUNC_PF10_ADC3_IN8 },

	/* ADC1 inputs - support for using on ADC1, 2, or sometimes 3, but
	 * we'll just stick to using ADC1 for these signals.
	 */
	{ STM32_PIN_PC0, STM32F1_PINMUX_FUNC_PC0_ADC123_IN10 },
	{ STM32_PIN_PC1, STM32F1_PINMUX_FUNC_PC1_ADC123_IN11 },
	{ STM32_PIN_PC2, STM32F1_PINMUX_FUNC_PC2_ADC123_IN12 },
	{ STM32_PIN_PC3, STM32F1_PINMUX_FUNC_PC3_ADC123_IN13 },
	{ STM32_PIN_PC4, STM32F1_PINMUX_FUNC_PC4_ADC12_IN14 },
	/* Use STM32_CNF_IN_ANALOG due to a typo in pinmux_stm32f1.h, line 103.
	 * It has PC4 instead of PC5 for STM32F1_PINMUX_FUNC_PC5_ADC12_IN15.
	 * TODO(pfagerburg) change when PR #28336 is merged.
	 */
	{ STM32_PIN_PC5, STM32_CNF_IN_ANALOG },


	/* USART/UART interfaces */
	{ STM32_PIN_PA9,  STM32F1_PINMUX_FUNC_PA9_USART1_TX },
	{ STM32_PIN_PA10, STM32F1_PINMUX_FUNC_PA10_USART1_RX },

	{ STM32_PIN_PA2, STM32F1_PINMUX_FUNC_PA2_USART2_TX },
	{ STM32_PIN_PA3, STM32F1_PINMUX_FUNC_PA3_USART2_RX },

	{ STM32_PIN_PC10, STM32F1_PINMUX_FUNC_PC10_UART4_TX },
	{ STM32_PIN_PC11, STM32F1_PINMUX_FUNC_PC11_UART4_RX },

	/* Use STM32_PIN_USART_TX/RX because the UART5 options aren't defined
	 * in pinmux_stm32f1.h
	 * TODO(pfagerburg) change when PR #28336 is merged.
	 */
	{ STM32_PIN_PC12, STM32_PIN_USART_TX },
	{ STM32_PIN_PD2, STM32_PIN_USART_RX },

	/* SPI bus */
	{ STM32_PIN_PA4, STM32F1_PINMUX_FUNC_PA4_SPI1_MASTER_NSS },
	{ STM32_PIN_PA5, STM32F1_PINMUX_FUNC_PA5_SPI1_MASTER_SCK },
	{ STM32_PIN_PA6, STM32F1_PINMUX_FUNC_PA6_SPI1_MASTER_MISO },
	{ STM32_PIN_PA7, STM32F1_PINMUX_FUNC_PA7_SPI1_MASTER_MOSI },

	/* I2C buses */
	{ STM32_PIN_PB6, STM32F1_PINMUX_FUNC_PB6_I2C1_SCL },
	{ STM32_PIN_PB7, STM32F1_PINMUX_FUNC_PB7_I2C1_SDA },

	{ STM32_PIN_PB10, STM32F1_PINMUX_FUNC_PB10_I2C2_SCL },
	{ STM32_PIN_PB11, STM32F1_PINMUX_FUNC_PB11_I2C2_SDA },

	/* USB interface */
	{ STM32_PIN_PA11, STM32F1_PINMUX_FUNC_PA11_USB_DM },
	{ STM32_PIN_PA12, STM32F1_PINMUX_FUNC_PA12_USB_DP },
};

static int pinmux_stm32_init(const struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
