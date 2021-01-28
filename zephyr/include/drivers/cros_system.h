/* Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CROS_SYSTEM_H_
#define ZEPHYR_INCLUDE_DRIVERS_CROS_SYSTEM_H_

#include <kernel.h>
#include <device.h>

/**
 * @brief CROS system driver APIs
 */

enum system_reset_cause {
	POWERUP = 0,
	VCC1_RST_PIN = 1,
	DEBUG_RST = 2,
	WATCHDOG_RST = 3,
	UNKNOWN_RST,
};

typedef int (*cros_system_get_reset_cause_api)(const struct device *dev);

__subsystem struct cros_system_driver_api {
	cros_system_get_reset_cause_api get_reset_cause;
};

/**
 * @brief Get the chip reset cause
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval non-negative if successful.
 * @retval Negative errno code if failure.
 */
__syscall int cros_system_get_reset_cause(const struct device *dev);

static inline int z_impl_cros_system_get_reset_cause(const struct device *dev)
{
	const struct cros_system_driver_api *api =
		(const struct cros_system_driver_api *)dev->api;

	if (!api->get_reset_cause) {
		return -ENOTSUP;
	}

	return api->get_reset_cause(dev);
}

/**
 * @}
 */
#include <syscalls/cros_system.h>
#endif /* ZEPHYR_INCLUDE_DRIVERS_CROS_SYSTEM_H_ */
