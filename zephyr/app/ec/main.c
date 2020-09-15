/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <sys/printk.h>
#include <zephyr.h>

void main(void)
{
	printk("Hello from a Chrome EC!\n");
	printk("  BOARD=%s\n", CONFIG_BOARD);
	printk("  ACTIVE_COPY=%s\n", CONFIG_CHROME_EC_ACTIVE_COPY);
}
