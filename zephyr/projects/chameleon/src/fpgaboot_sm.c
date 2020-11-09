/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdbool.h>
#include <zephyr.h>
#include <sys/__assert.h>
#include "fpgaboot_sm.h"

static void run_fpgaboot_init(enum fpga_event event, enum fpga_state *p_state,
			      uint32_t *p_actions)
{
	*p_state = FPGABOOT_OFF;
	*p_actions = FPGABOOT_DEASSERT_SOM_PWR_EN |
		     FPGABOOT_ASSERT_SOM_POR_L_LOAD_L;
}

static void run_fpgaboot_off(enum fpga_event event, enum fpga_state *p_state,
			     uint32_t *p_actions)
{
	if (event == FPGABOOT_POWER_ON_REQ) {
		*p_state = FPGABOOT_PWR_GOOD;
		*p_actions = FPGABOOT_ASSERT_SOM_PWR_EN |
			     FPGABOOT_START_SHORT_TIMER;
	}
}

static void run_fpgaboot_pwr_good(enum fpga_event event,
				  enum fpga_state *p_state, uint32_t *p_actions)
{
	if (event == FPGABOOT_POWER_OFF_REQ ||
	    event == FPGABOOT_TIMER_EXPIRED) {
		*p_state = FPGABOOT_OFF;
		*p_actions = FPGABOOT_DEASSERT_SOM_PWR_EN |
			     FPGABOOT_ASSERT_SOM_POR_L_LOAD_L |
			     FPGABOOT_STOP_TIMER;
	} else if (event == FPGABOOT_SOM_PWR_GOOD_ASSERTED) {
		*p_state = FPGABOOT_LOAD;
		*p_actions = FPGABOOT_DEASSERT_SOM_POR_L_LOAD_L |
			     FPGABOOT_START_LONG_TIMER;
	}
}

static void run_fpgaboot_load(enum fpga_event event, enum fpga_state *p_state,
			      uint32_t *p_actions)
{
	if (event == FPGABOOT_POWER_OFF_REQ ||
	    event == FPGABOOT_TIMER_EXPIRED ||
	    event == FPGABOOT_SOM_PWR_GOOD_DEASSERTED) {
		*p_state = FPGABOOT_OFF;
		*p_actions = FPGABOOT_DEASSERT_SOM_PWR_EN |
			     FPGABOOT_ASSERT_SOM_POR_L_LOAD_L |
			     FPGABOOT_STOP_TIMER;
	} else if (event == FPGABOOT_SOM_FPGA_DONE_ASSERTED) {
		*p_state = FPGABOOT_RUNNING;
		*p_actions = FPGABOOT_STOP_TIMER;
	}
}

static void run_fpgaboot_running(enum fpga_event event,
				 enum fpga_state *p_state, uint32_t *p_actions)
{
	if (event == FPGABOOT_POWER_OFF_REQ ||
	    event == FPGABOOT_SOM_PWR_GOOD_DEASSERTED) {
		*p_state = FPGABOOT_OFF;
		*p_actions = FPGABOOT_DEASSERT_SOM_PWR_EN |
			     FPGABOOT_ASSERT_SOM_POR_L_LOAD_L;
	}
}

void fpgaboot_init_state_machine(enum fpga_state *p_state, uint32_t *p_actions)
{
	__ASSERT(p_state != NULL, "NULL pointer");
	__ASSERT(p_actions != NULL, "NULL pointer");

	*p_state = FPGABOOT_INIT;
	/* Run the state machine to get from INIT to OFF. */
	fpgaboot_run_state_machine(FPGABOOT_EVENT_NONE, p_state, p_actions);
}

void fpgaboot_run_state_machine(enum fpga_event event, enum fpga_state *p_state,
				uint32_t *p_actions)
{
	__ASSERT(p_state != NULL, "NULL pointer");
	__ASSERT(p_actions != NULL, "NULL pointer");

	*p_actions = 0;
	switch (*p_state) {
	case FPGABOOT_INIT:
		run_fpgaboot_init(event, p_state, p_actions);
		break;
	case FPGABOOT_OFF:
		run_fpgaboot_off(event, p_state, p_actions);
		break;
	case FPGABOOT_PWR_GOOD:
		run_fpgaboot_pwr_good(event, p_state, p_actions);
		break;
	case FPGABOOT_LOAD:
		run_fpgaboot_load(event, p_state, p_actions);
		break;
	case FPGABOOT_RUNNING:
		run_fpgaboot_running(event, p_state, p_actions);
		break;
	default:
		__ASSERT(false, "Invalid state");
	}
}

bool fpgaboot_is_steady_state(enum fpga_state state)
{
	return state == FPGABOOT_OFF || state == FPGABOOT_RUNNING;
}
