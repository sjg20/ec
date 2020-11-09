/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Implement a state machine for powering on, booting, and powering off the
 * FPGA System-on-Module (SOM) that is the heart of Chameleon v3.
 * See doc/fpga_sm.md for design details.
 */

#ifndef __FPGABOOT_SM_H
#define __FPGABOOT_SM_H

#include <stdbool.h>
#include <zephyr.h>

enum fpga_state {
	FPGABOOT_INIT,
	FPGABOOT_OFF,
	FPGABOOT_PWR_GOOD,
	FPGABOOT_LOAD,
	FPGABOOT_RUNNING,
};

enum fpga_event {
	FPGABOOT_EVENT_NONE,
	FPGABOOT_POWER_ON_REQ,
	FPGABOOT_POWER_OFF_REQ,
	FPGABOOT_SOM_PWR_GOOD_ASSERTED,
	FPGABOOT_SOM_PWR_GOOD_DEASSERTED,
	FPGABOOT_SOM_FPGA_DONE_ASSERTED,
	FPGABOOT_SOM_FPGA_DONE_DEASSERTED,
	FPGABOOT_TIMER_EXPIRED,
};

#define FPGABOOT_ACTION_NONE 0x00
#define FPGABOOT_ASSERT_SOM_PWR_EN 0x01
#define FPGABOOT_DEASSERT_SOM_PWR_EN 0x02
/* NB: SOM_POR_L_LOAD_L is active low, so "assert" == "set to 0V" */
#define FPGABOOT_ASSERT_SOM_POR_L_LOAD_L 0x04
#define FPGABOOT_DEASSERT_SOM_POR_L_LOAD_L 0x08
#define FPGABOOT_START_SHORT_TIMER 0x10
#define FPGABOOT_START_LONG_TIMER 0x20
#define FPGABOOT_STOP_TIMER 0x40

/**
 * @brief Initialize the state machine data
 *
 * @param p_state pointer to an `fpga_state` variable
 * @param p_actions pointer to variable to set bit flags for actions.
 */
void fpgaboot_init_state_machine(enum fpga_state *p_state, uint32_t *p_actions);

/**
 * @brief Run the state machine
 *
 * The caller should set `event` to indicate the event being processed.
 * The state machine may change `*p_state` and set one or more bits in
 * `*p_actions` to indicate what the caller should do next.
 *
 * @param p_state pointer to an `fpga_state` variable
 * @param event the event being processed
 * @param p_actions pointer to variable to set bit flags for actions.
 */
void fpgaboot_run_state_machine(enum fpga_state *p_state, enum fpga_event event,
				uint32_t *p_actions);

/**
 * @brief Determine if the FPGA is in a steady state, on or off
 *
 * @param state an `fpga_state` variable
 * @retval true if the FPGA is either on or off, false if it's in a
 * different state moving towards either on or off.
 */
bool fpgaboot_is_steady_state(enum fpga_state state);

#endif
