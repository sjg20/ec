/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This file contains only initialization functions for the Si5338 clock
 * generator chip. As such, there is only a SYS_INIT, and no public API.
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/i2c.h>
#include <sys/printk.h>

/*
 * The generated header uses a non-standard keyword "code" to place the
 * data in flash. It is sufficient in our app to just omit that keyword.
 */
#define code
#include "Si5338-CLK24-Registers.h"

/* Si5338's I2C address. */
#define SI5338_DEVICE 0x71

/* Names of Si5338 registers that we need to specifically read or write. */
#define FCAL_OVRD1 45
#define FCAL_OVRD2 46
#define FCAL_OVRD3 47
#define PLL_CFG2 49
#define ALARMS 218
#define OUTPUT_ENABLES 230
#define FCAL1 235
#define FCAL2 236
#define FCAL3 237
#define DIS_LOL 241
#define SOFT_RESET 246

/*
 * @brief Read an Si5338 register
 *
 * @param i2c_dev Device pointer for the I2C device
 * @param addr Register address
 *
 * @retval The value from the register
 */
static uint8_t si5338_read(const struct device *i2c_dev, uint8_t addr)
{
	uint8_t val;

	i2c_burst_read(i2c_dev, SI5338_DEVICE, addr, &val, 1);
	return val;
}

/*
 * @brief Write to an Si5338 register, possibly with a mask
 *
 * Write to a register in the Si5338. In the case of a mask, perform
 * a read/modify/write.
 *
 * This algorithm (and comments) provided in the Si5338 datasheet, showing
 * how to use the mask and value to determine what to write.
 *
 * @param i2c_dev Device pointer for the I2C device
 * @param addr Register address
 * @param val Value to write
 * @param mask Bits to write. 0xFF to write the entire byte, something
 * other than 0xFF to read the register, mask the bits, and write the
 * resulting value.
 */
static void si5338_write(const struct device *i2c_dev, uint8_t addr,
			 uint8_t val, uint8_t mask)
{
	if (mask == 0xFF) {
		i2c_burst_write(i2c_dev, SI5338_DEVICE, addr, &val, 1);
	} else {
		/* Do a read-modify-write using I2C and bit-wise operations. */
		uint8_t curr_val = si5338_read(i2c_dev, addr);
		uint8_t new_val = (curr_val & (~mask)) | (val & mask);

		i2c_burst_write(i2c_dev, SI5338_DEVICE, addr, &new_val, 1);
	}
}

/**
 * @brief Initialize the Si5338 with data provided by Clock Builder Pro.
 *
 * The Si5338 has a lot of registers, but Silicon Lab's Clock Builder Pro
 * tool provides a GUI (Windows-only, ofc) that walks through all the
 * options and produces a C header file with all of the register values.
 * The header is checked in as-is, with no reformatting, no adherence to
 * coding conventions, no additional license tags, nothing. That way when
 * we generate a new file, the diffs are a lot easier.
 */
static int init_si5338(const struct device *ptr)
{
	ARG_UNUSED(ptr);

	const struct device *i2c_dev = device_get_binding("I2C_2");

	if (!i2c_dev) {
		printk("%s: I2C_2 device not found.\n", __func__);
		return -ENODEV;
	}

	/*
	 * Follow the programming procedure laid out in Figure 9 of the
	 * Si5338 datasheet.
	 */

	/* Disable all outputs; set OEB_ALL = 1; reg230[4] */
	si5338_write(i2c_dev, OUTPUT_ENABLES, 0x10, 0x10);
	/* Pause LOL; set DIS_LOL = 1; reg241[7] */
	si5338_write(i2c_dev, DIS_LOL, 0x80, 0x80);
	/* Write new configuration */
	for (int i = 0; i < NUM_REGS_MAX; ++i) {
		if (Reg_Store[i].Reg_Mask) {
			si5338_write(i2c_dev, Reg_Store[i].Reg_Addr,
				     Reg_Store[i].Reg_Val,
				     Reg_Store[i].Reg_Mask);
		}
	}
	/* Wait for input clock valid
	 * "Input clocks are validated with the LOS alarms. See
	 * Register 218 to determine which LOS should be monitored"
	 *
	 * I'm going to check bit 0 (SYS_CAL) and bit 2 (LOS_CLKIN).
	 * Chameleon v3 doesn't use IN4/5/6, so LOS_FDBK won't matter,
	 * and we haven't set up the PLL yet, so of course PLL_LOL
	 * shouldn't matter.
	 */
	while (si5338_read(i2c_dev, ALARMS) & 0x05) {
		k_msleep(1);
	}
	/* Configure PLL for locking; set FCAL_OVRD_EN = 0; reg49[7] */
	si5338_write(i2c_dev, PLL_CFG2, 0x00, 0x80);
	/* Initiate locking of PLL; set SOFT_RESET = 1; reg246[1] */
	si5338_write(i2c_dev, SOFT_RESET, 0x02, 0x02);
	/* Wait 25 ms */
	k_msleep(25);
	/* Restart LOL; set DIS_LOL = 0; reg241[7]; set reg241 = 0x65 */
	si5338_write(i2c_dev, DIS_LOL, 0x80 | 0x65, 0xFF);
	/*
	 * Wait for PLL lock; "PLL is locked when PLL_LOL, SYS_CAL, and all
	 * other alarms are cleared"
	 *
	 * Note that Chameleon v3 doesn't use IN4/5/6, so ignore LOS_FDBK
	 */
	while (si5338_read(i2c_dev, ALARMS) & 0x15) {
		k_msleep(1);
	}
	/*
	 * Copy FCAL values to registers
	 * 237[1:0] to 47[1:0]
	 * 236[7:0] to 46[7:0]
	 * 235[7:0] to 45[7:0]
	 * Set 47[7:2] = 000101b
	 */
	si5338_write(i2c_dev, FCAL_OVRD3, si5338_read(i2c_dev, FCAL3), 0x03);
	si5338_write(i2c_dev, FCAL_OVRD2, si5338_read(i2c_dev, FCAL2), 0xFF);
	si5338_write(i2c_dev, FCAL_OVRD1, si5338_read(i2c_dev, FCAL1), 0xFF);
	si5338_write(i2c_dev, FCAL_OVRD3, 0x14, 0xFC);
	/* Set PLL to use FCAL values; set FCAL_OVRD_END = 1; reg49[7] */
	si5338_write(i2c_dev, PLL_CFG2, 0x80, 0x80);
	/* Optional step for down-spread (N/A for us) */
	/* Enable outputs; set OEB_ALL = 0; reg230[4] */
	si5338_write(i2c_dev, OUTPUT_ENABLES, 0x1F, 0x1F);

	return 0;
}
SYS_INIT(init_si5338, APPLICATION, 5);
