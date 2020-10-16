/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __SD_H
#define __SD_H

#include <zephyr.h>

enum sd_state {
	SD_MUX_OFF,	/**< Disconnect the SD card. */
	SD_MUX_FPGA,	/**< Connect the SD card to the FPGA. */
	SD_MUX_USB,	/**< Connect the SD card to the USB controller. */
};

/**
 * @brief Set the state of the SD card mux.
 *
 * This function sets the SD card mux signal lines to disconnect the SD
 * card signals, or to connect them to either the FPGA or the USB controller.
 *
 * @param state One of the values from sd_state
 *
 * @retval 0 if successful.
 * @retval -ENODEV or error return from Zephyr API functions.
 */
int sd_mux_set(enum sd_state state);

/** @brief Get the state of the SD card mux.
 *
 * @retval The current sd_state.
 */
enum sd_state sd_mux_get(void);

/**
 * @brief Return the state of the SD's CD_DET signal.
 *
 * @retval 0 if the SD card is inserted, 1 if not, negative value of
 * an errno.h constant if there was a failure.
 */
int sd_get_cd_det(void);

/**
 * @brief Initialize the SD mux GPIOs to a disconnected state
 *
 * The SD mux controls whether the SD card signals are connected, and
 * if they are connected, whether to the FPGA or the USB controller.
 * Set up the GPIOs and then set the mux to disconnected.
 *
 * @retval 0 if successful.
 * @retval -ENODEV or error return from Zephyr API functions.
 */
int sd_mux_init(void);

#endif
