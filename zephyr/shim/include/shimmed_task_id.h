/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_SHIMMED_TASK_ID_H
#define __CROS_EC_SHIMMED_TASK_ID_H

#include "common.h"

/* Task identifier (8 bits) */
typedef uint8_t task_id_t;

/*
 * Highest priority on bottom -- same as in platform/ec. List of CROS_EC_TASK
 * items. See CONFIG_TASK_LIST in platform/ec's config.h for more information.
 * This will only automatically get generated if CONFIG_ZTEST is not defined.
 * Unit tests must define their own tasks.
 */
#ifndef CONFIG_ZTEST
#define CROS_EC_TASK_LIST                                                 \
	COND_CODE_1(HAS_TASK_CHG_RAMP,                                    \
		     (CROS_EC_TASK(CHG_RAMP, chg_ramp_task, 0,            \
				   CONFIG_TASK_CHG_RAMP_STACK_SIZE)), ()) \
	COND_CODE_1(HAS_TASK_USB_CHG_P0,                                  \
		     (CROS_EC_TASK(USB_CHG_P0, usb_charger_task, 0,       \
				   CONFIG_TASK_USB_CHG_STACK_SIZE)), ())  \
	COND_CODE_1(HAS_TASK_USB_CHG_P1,                                  \
		     (CROS_EC_TASK(USB_CHG_P1, usb_charger_task, 0,       \
				   CONFIG_TASK_USB_CHG_STACK_SIZE)), ())  \
	COND_CODE_1(HAS_TASK_USB_CHG_P2,                                  \
		     (CROS_EC_TASK(USB_CHG_P2, usb_charger_task, 0,       \
				   CONFIG_TASK_USB_CHG_STACK_SIZE)), ())  \
	COND_CODE_1(HAS_TASK_CHARGER,                                     \
		     (CROS_EC_TASK(CHARGER, charger_task, 0,              \
				   CONFIG_TASK_CHARGER_STACK_SIZE)), ())  \
	COND_CODE_1(HAS_TASK_CHIPSET,                                     \
		     (CROS_EC_TASK(CHIPSET, chipset_task, 0,              \
				   CONFIG_TASK_CHIPSET_STACK_SIZE)), ())  \
	COND_CODE_1(HAS_TASK_MOTIONSENSE,                                     \
		     (CROS_EC_TASK(MOTIONSENSE, motion_sense_task, 0,         \
				   CONFIG_TASK_MOTIONSENSE_STACK_SIZE)), ())  \
	COND_CODE_1(HAS_TASK_HOOKS,                                       \
			(CROS_EC_TASK(HOOKS, hook_task, 0,                \
				CONFIG_TASK_HOOKS_STACK_SIZE)), ())       \
	COND_CODE_1(HAS_TASK_HOSTCMD,                                     \
		     (CROS_EC_TASK(HOSTCMD, host_command_task, 0,         \
				   CONFIG_TASK_HOSTCMD_STACK_SIZE)), ())  \
	COND_CODE_1(HAS_TASK_KEYPROTO,                                    \
		     (CROS_EC_TASK(KEYPROTO, keyboard_protocol_task, 0,   \
				   CONFIG_TASK_KEYPROTO_STACK_SIZE)), ()) \
	COND_CODE_1(HAS_TASK_POWERBTN,                                    \
		     (CROS_EC_TASK(POWERBTN, power_button_task, 0,        \
				   CONFIG_TASK_POWERBTN_STACK_SIZE)), ()) \
	COND_CODE_1(HAS_TASK_KEYSCAN,                                     \
		     (CROS_EC_TASK(KEYSCAN, keyboard_scan_task, 0,        \
				   CONFIG_TASK_KEYSCAN_STACK_SIZE)), ())  \
	COND_CODE_1(HAS_TASK_PD_C0,                                       \
		     (CROS_EC_TASK(PD_C0, pd_task, 0,                     \
				   CONFIG_TASK_PD_STACK_SIZE)), ())       \
	COND_CODE_1(HAS_TASK_PD_C1,                                       \
		     (CROS_EC_TASK(PD_C1, pd_task, 0,                     \
				   CONFIG_TASK_PD_STACK_SIZE)), ())       \
	COND_CODE_1(HAS_TASK_PD_C2,                                       \
		     (CROS_EC_TASK(PD_C2, pd_task, 0,                     \
				   CONFIG_TASK_PD_STACK_SIZE)), ())       \
	COND_CODE_1(HAS_TASK_PD_C3,                                       \
		     (CROS_EC_TASK(PD_C3, pd_task, 0,                     \
				   CONFIG_TASK_PD_STACK_SIZE)), ())       \
	COND_CODE_1(HAS_TASK_PD_INT_C0,                                   \
		     (CROS_EC_TASK(PD_INT_C0, pd_interrupt_handler_task, 0, \
				   CONFIG_TASK_PD_INT_STACK_SIZE)), ())   \
	COND_CODE_1(HAS_TASK_PD_INT_C1,                                   \
		     (CROS_EC_TASK(PD_INT_C1, pd_interrupt_handler_task, 0, \
				   CONFIG_TASK_PD_INT_STACK_SIZE)), ())   \
	COND_CODE_1(HAS_TASK_PD_INT_C2,                                   \
		     (CROS_EC_TASK(PD_INT_C2, pd_interrupt_handler_task, 0, \
				   CONFIG_TASK_PD_INT_STACK_SIZE)), ())   \
	COND_CODE_1(HAS_TASK_PD_INT_C3,                                   \
		     (CROS_EC_TASK(PD_INT_C3, pd_interrupt_handler_task, 0, \
				   CONFIG_TASK_PD_INT_STACK_SIZE)), ())
#elif defined(CONFIG_HAS_TEST_TASKS)
#include "shimmed_test_tasks.h"
/*
 * There are two different ways to define a task list (because historical
 * reasons). Applications use CROS_EC_TASK_LIST to define their tasks, while
 * unit tests that need additional tasks use CONFIG_TEST_TASK_LIST. For
 * shimming a unit test, define CROS_EC_TASk_LIST as whatever
 * CONFIG_TEST_TASK_LIST expands to.
 */
#if defined(CONFIG_TEST_TASK_LIST) && !defined(CROS_EC_TASK_LIST)
#define CROS_EC_TASK_LIST CONFIG_TEST_TASK_LIST
#endif /* CONFIG_TEST_TASK_LIST && !CROS_EC_TASK_LIST */
#endif /* !CONFIG_ZTEST */

#ifndef CROS_EC_TASK_LIST
#define CROS_EC_TASK_LIST
#endif /* CROS_EC_TASK_LIST */

/*
 * Define the task_ids globally for all shimmed platform/ec code to use.
 * Note that unit test task lists use TASK_TEST, which we can just alias
 * into a regular CROS_EC_TASK.
 */
#define CROS_EC_TASK(name, ...) TASK_ID_##name,
#define TASK_TEST(name, ...) CROS_EC_TASK(name)
enum {
	TASK_ID_IDLE = -1, /* We don't shim the idle task */
	CROS_EC_TASK_LIST
	TASK_ID_COUNT,
	TASK_ID_INVALID = 0xff, /* Unable to find the task */
};
#undef CROS_EC_TASK
#undef TASK_TEST

#endif /* __CROS_EC_SHIMMED_TASK_ID_H */
