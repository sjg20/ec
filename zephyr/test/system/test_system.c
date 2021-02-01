/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <device.h>
#include <drivers/cros_bbram.h>
#include <logging/log.h>
#include <ztest.h>

#include "bbram.h"
#include "system.h"

LOG_MODULE_REGISTER(test);

static char mock_data[64] =
	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@";

static int mock_bbram_read(const struct device *unused, int offset, int size,
			   char *data)
{
	if (offset < 0 || offset + size >= ARRAY_SIZE(mock_data))
		return -1;
	memcpy(data, mock_data + offset, size);
	return EC_SUCCESS;
}

static const struct cros_bbram_driver_api bbram_api = {
	.ibbr = NULL,
	.reset_ibbr = NULL,
	.vsby = NULL,
	.reset_vsby = NULL,
	.vcc1 = NULL,
	.reset_vcc1 = NULL,
	.read = mock_bbram_read,
	.write = NULL,
};

static const struct device bbram_dev_instance = {
	.name = "TEST_BBRAM_DEV",
	.config = NULL,
	.api = &bbram_api,
	.data = NULL,
};

const struct device *bbram_dev = &bbram_dev_instance;

static void test_bbram_get(void)
{
	uint8_t output[10];
	int rc;

	rc = system_get_bbram(SYSTEM_BBRAM_IDX_VBNVBLOCK0, output);
	zassert_equal(rc, 0, NULL);
	zassert_mem_equal(output, mock_data + BBRM_DATA_INDEX_VBNVCNTXT, 1,
			  NULL);

	rc = system_get_bbram(SYSTEM_BBRAM_IDX_VBNVBLOCK0 + 5, output);
	zassert_equal(rc, 0, NULL);
	zassert_mem_equal(output, mock_data + BBRM_DATA_INDEX_VBNVCNTXT + 5, 1,
			  NULL);

	rc = system_get_bbram(SYSTEM_BBRAM_IDX_PD0, output);
	zassert_equal(rc, 0, NULL);
	zassert_mem_equal(output, mock_data + BBRM_DATA_INDEX_PD0, 1, NULL);

	rc = system_get_bbram(SYSTEM_BBRAM_IDX_PD1, output);
	zassert_equal(rc, 0, NULL);
	zassert_mem_equal(output, mock_data + BBRM_DATA_INDEX_PD1, 1, NULL);

	rc = system_get_bbram(SYSTEM_BBRAM_IDX_PD2, output);
	zassert_equal(rc, 0, NULL);
	zassert_mem_equal(output, mock_data + BBRM_DATA_INDEX_PD2, 1, NULL);

	rc = system_get_bbram(SYSTEM_BBRAM_IDX_TRY_SLOT, output);
	zassert_equal(rc, 0, NULL);
	zassert_mem_equal(output, mock_data + BBRM_DATA_INDEX_TRY_SLOT, 1,
			  NULL);
}

void test_main(void)
{
	ztest_test_suite(system, ztest_unit_test(test_bbram_get));
	ztest_run_test_suite(system);
}
