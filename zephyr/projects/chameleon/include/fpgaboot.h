/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __FPGABOOT_H
#define __FPGABOOT_H

#include <zephyr.h>

/**
 * Define the boot source for the FPGA
 *
 * The Enclustra FPGA SOM (system-on-module) defines two pins, BOOT_MODE[1:0]
 * to control the boot source for the FPGA. The module includes an EMMC
 * flash (essentially an SD card in an IC package), and a quad-SPI flash
 * memory. The module also has SDIO signals that the carrier board can
 * route to an SD/microSD socket (which Chameleon v3 does).
 *
 * @note The default boot mode on Chameleon v3 is SDIO.
 */
enum fpga_boot_mode { FPGA_BOOT_EMMC, FPGA_BOOT_QSPI, FPGA_BOOT_SDIO };

/*
 * @brief Set the boot mode for the FPGA SOM
 *
 * @note Callers are allowed to set the boot mode only when the FPGA is not
 * busy with a power-up or power-down operation.
 *
 * @param mode One of the `fpga_boot_mode` values
 * @retval 0
 * No errors.
 * @retval -EBUSY
 * The FPGA is busy powering up or down
 * @retval -EINVAL
 * Invalid value, probably from passing an int instead of enum
 */
int fpgaboot_set_boot_mode(enum fpga_boot_mode mode);

/*
 * @brief Send a request to power on
 *
 * @note The caller should call `fpgaboot_set_boot_mode` before this unless
 * they want to use the default value (which is `FPGA_BOOT_SDIO`)
 *
 * @retval 0
 * No errors.
 * @retval -ENOMSG
 * Message was not posted.
 * @retval -EAGAIN
 * Waiting period timed out.
 */
int fpgaboot_power_on(void);

/*
 * @brief Send a request to power off
 *
 * @retval 0
 * No errors.
 * @retval -ENOMSG
 * Message was not posted.
 * @retval -EAGAIN
 * Waiting period timed out.
 */
int fpgaboot_power_off(void);

#endif
