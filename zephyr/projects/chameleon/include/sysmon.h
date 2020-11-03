/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __SYSMON_H
#define __SYSMON_H

#include <zephyr.h>

/**
 * @brief Define all of the monitored inputs.
 */
enum sysmon_input {
	SYSMON_VMON_12V = 0,	/**< VMON_12V, scaled to 3V. */
	SYSMON_CMON_SOM,	/**< CMON_PP12000_SOM, scaled to 1V/A. */
	SYSMON_CMON_BOARD,	/**< CMON_PP12000_CHAMELEON, scaled to 1V/A. */

	/* These are connected to SYSMON1..8 through the "NO" terminals of the
	 * analog switches (SYSMON_SEL_REGULATORS).
	 */
	SYSMON_VMON_PP1200,	/**< PP1200, the 1.2V supply. */
	SYSMON_VMON_PP1360,	/**< PP1360, the 1.36V supply. */
	SYSMON_VMON_PP1260,	/**< PP1260, the 1.26V supply. */
	SYSMON_VMON_PP1000,	/**< PP1000, the 1.00V supply. */
	SYSMON_VMON_9V,		/**< VMON_9V, scaled to 2.25V. */
	SYSMON_VMON_5V,		/**< VMON_5V, scaled to 1.25V. */
	SYSMON_VMON_3V3,	/**< VMON_3V3, scaled to 2.2V. */
	SYSMON_VMON_PP1800,	/**< PP1800, the 1.8V supply. */

	/* These are connected to SYSMON1..6 through the "NC" terminals of the
	 * analog switches (SYSMON_SEL_SOM). The "NC" terminals on the analog
	 * switches for SYSMON7 and SYSMON8 are no-connect.
	 */
	SYSMON_SOM_VMON,	/**< VMON on the SOM. */
	SYSMON_SOM_VMON_1V2,	/**< VMON_1V2 on the SOM. */
	SYSMON_SOM_VMON_MGT,	/**< VMON_MGT on the SOM. */
	SYSMON_SOM_VMON_1V8A,	/**< VMON_1V8A on the SOM. */
	SYSMON_SOM_VMON_C8,	/**< "VMON_C8" on the SOM. */
	SYSMON_SOM_VREF_CS,	/**< VREF_CS (CS=current sense), 0V. */

	SYSMON_VMON_NUM_INPUTS	/**< Number of monitored inputs. */
};

/**
 * @brief Get the value of any of the monitored inputs
 *
 * This function will return the interpreted value of any of the monitored
 * inputs, so anything that is scaled into the ADC will be scaled back up
 * to the range of the actual signal.
 *
 * @param input Which of the monitored inputs to return the value.
 * @retval The value of the monitored input.
 */
float sysmon_get_val(enum sysmon_input input);

#endif
