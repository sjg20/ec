/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Renesas (Intersil) ISL-9237/38 battery charger public header
 */

#ifndef __CROS_EC_DRIVER_CHARGER_ISL923X_PUBLIC_H
#define __CROS_EC_DRIVER_CHARGER_ISL923X_PUBLIC_H

#define ISL923X_ADDR_FLAGS	(0x09)

extern const struct charger_drv isl923x_drv;

/**
 * Initialize AC & DC prochot threshold
 *
 * @param	chgnum: Index into charger chips
 * @param	AC Prochot threshold current in mA:
 *			multiple of 128 up to 6400 mA
 *			DC Prochot threshold current in mA:
 *			multiple of 128 up to 12800 mA
 * 		Bits below 128mA are truncated (ignored).
 * @return enum ec_error_list
 */
int isl923x_set_ac_prochot(int chgnum, uint16_t ma);
int isl923x_set_dc_prochot(int chgnum, uint16_t ma);

/**
 * Set the general comparator output polarity when asserted.
 *
 * @param chgnum: Index into charger chips
 * @param invert: Non-zero to invert polarity, zero to non-invert.
 * @return EC_SUCCESS, error otherwise.
 */
int isl923x_set_comparator_inversion(int chgnum, int invert);

/**
 * Prepare the charger IC for battery ship mode.  Battery ship mode sets the
 * lowest power state for the IC. Battery ship mode can only be entered from
 * battery only mode.
 *
 * @param chgnum index into chg_chips table.
 */
void raa489000_hibernate(int chgnum, bool disable_adc);
enum ec_error_list isl9238c_hibernate(int chgnum);
enum ec_error_list isl9238c_resume(int chgnum);

#endif /* __CROS_EC_DRIVER_CHARGER_ISL923X_PUBLIC_H */
