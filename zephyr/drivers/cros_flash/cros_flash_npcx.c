/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_cros_flash

#include <dt-bindings/clock/npcx_clock.h>
#include <drivers/cros_flash.h>
#include <drivers/clock_control.h>
#include <drivers/gpio.h>
#include <kernel.h>
#include <logging/log.h>
#include <soc.h>
#include <soc/nuvoton_npcx/reg_def_cros.h>
#include <sys/__assert.h>
#include "ec_tasks.h"
#include "soc_miwu.h"
#include "task.h"
#include "../drivers/flash/spi_nor.h"

LOG_MODULE_REGISTER(cros_flash, LOG_LEVEL_ERR);

/* Device config */
struct cros_flash_npcx_config {
	/* flash interface unit base address */
	uintptr_t base;
	/* clock configuration */
	struct npcx_clk_cfg clk_cfg;
	/* Flash size (Unit:bytes) */
	int size;
	/* pinmux configuration */
	const uint8_t alts_size;
	const struct npcx_alt *alts_list;
};

/* Device data */
struct cros_flash_npcx_data {
	/* flag of flash write protection */
	bool write_protectied;
	/* mutex of flash interface controller */
	struct k_sem lock_sem;
};

/* TODO: Should we replace them with Kconfig variables */
#define CONFIG_FLASH_WRITE_SIZE 0x1 /* minimum write size */
#define CONFIG_FLASH_WRITE_IDEAL_SIZE 256 /* one page size for write */

/* TODO: It should be defined in the spi_nor.h in the zephyr repository */
#define SPI_NOR_CMD_FAST_READ 0x0B

/* Driver convenience defines */
#define DRV_CONFIG(dev) ((const struct cros_flash_npcx_config *)(dev)->config)
#define DRV_DATA(dev) ((struct cros_flash_npcx_data *)(dev)->data)
#define HAL_INSTANCE(dev) (struct fiu_reg *)(DRV_CONFIG(dev)->base)

/* cros ec flash local inline functions */
static inline void cros_flash_npcx_mutex_lock(const struct device *dev)
{
	struct cros_flash_npcx_data *data = DRV_DATA(dev);

	k_sem_take(&data->lock_sem, K_FOREVER);
}

static inline void cros_flash_npcx_mutex_unlock(const struct device *dev)
{
	struct cros_flash_npcx_data *data = DRV_DATA(dev);

	k_sem_give(&data->lock_sem);
}

static inline void cros_flash_npcx_set_address(const struct device *dev,
					       uint32_t qspi_addr)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);
	uint8_t *addr = (uint8_t *)&qspi_addr;

	/* Write 3 bytes address to UMA registers */
	inst->UMA_AB2 = addr[2];
	inst->UMA_AB1 = addr[1];
	inst->UMA_AB0 = addr[0];
}

static inline void cros_flash_npcx_cs_level(const struct device *dev, int level)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	/* Set chip select to high/low level */
	if (level == 0)
		inst->UMA_ECTS &= ~BIT(NPCX_UMA_ECTS_SW_CS1);
	else
		inst->UMA_ECTS |= BIT(NPCX_UMA_ECTS_SW_CS1);
}

static inline void cros_flash_npcx_exec_cmd(const struct device *dev,
					    uint8_t code, uint8_t cts)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

#ifdef CONFIG_ASSERT
	struct cros_flash_npcx_data *data = DRV_DATA(dev);

	/* Flash mutex must be held while executing UMA commands */
	__ASSERT((k_sem_count_get(&data->lock_sem) == 0), "UMA is not locked");
#endif

	/* set UMA_CODE */
	inst->UMA_CODE = code;
	/* execute UMA flash transaction */
	inst->UMA_CTS = cts;
	while (IS_BIT_SET(inst->UMA_CTS, NPCX_UMA_CTS_EXEC_DONE))
		;
}

static inline void cros_flash_npcx_burst_read(const struct device *dev,
					      char *dst_data, int dst_size)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	/* Burst read transaction */
	for (int idx = 0; idx < dst_size; idx++) {
		/* 1101 0101 - EXEC, RD, NO CMD, NO ADDR, 4 bytes */
		inst->UMA_CTS = UMA_CODE_RD_BYTE(1);
		/* wait for UMA to complete */
		while (IS_BIT_SET(inst->UMA_CTS, NPCX_UMA_CTS_EXEC_DONE))
			;
		/* Get read transaction results*/
		dst_data[idx] = inst->UMA_DB0;
	}
}

static inline int cros_flash_npcx_wait_busy_bit_clear(const struct device *dev)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);
	int wait_period = 10; /* 10 us period t0 check status register */
	int timeout = (10 * USEC_PER_SEC) / wait_period; /* 10 seconds */

	do {
		/* Read status register */
		inst->UMA_CTS = UMA_CODE_RD_BYTE(1);
		while (IS_BIT_SET(inst->UMA_CTS, NPCX_UMA_CTS_EXEC_DONE))
			;
		/* Status bit is clear */
		if ((inst->UMA_DB0 & SPI_NOR_WIP_BIT) == 0)
			break;
		k_usleep(wait_period);
	} while (--timeout); /* Wait for busy bit clear */

	if (timeout) {
		return 0;
	} else {
		return -ETIMEDOUT;
	}
}

/* cros ec flash local functions */
static int cros_flash_npcx_wait_ready(const struct device *dev)
{
	int ret = 0;

	/* Drive CS to low */
	cros_flash_npcx_cs_level(dev, 0);

	/* Command for Read status register of flash */
	cros_flash_npcx_exec_cmd(dev, SPI_NOR_CMD_RDSR, UMA_CODE_CMD_ONLY);
	/* Wait busy bit is clear */
	ret = cros_flash_npcx_wait_busy_bit_clear(dev);
	/* Drive CS to low */
	cros_flash_npcx_cs_level(dev, 1);

	return ret;
}

static int cros_flash_npcx_set_write_enable(const struct device *dev)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);
	int ret;

	/* Wait for previous operation to complete */
	ret = cros_flash_npcx_wait_ready(dev);
	if (ret != 0)
		return ret;

	/* Write enable command */
	cros_flash_npcx_exec_cmd(dev, SPI_NOR_CMD_WREN, UMA_CODE_CMD_ONLY);

	/* Wait for flash is not busy */
	ret = cros_flash_npcx_wait_ready(dev);
	if (ret != 0)
		return ret;

	if ((inst->UMA_DB0 & SPI_NOR_WEL_BIT) != 0)
		return 0;
	else
		return -EINVAL;
}

static void cros_flash_npcx_burst_write(const struct device *dev,
					unsigned int dest_addr,
					unsigned int bytes,
					const char *src_data)
{
	/* Chip Select down */
	cros_flash_npcx_cs_level(dev, 0);

	/* Set write address */
	cros_flash_npcx_set_address(dev, dest_addr);
	/* Start programming */
	cros_flash_npcx_exec_cmd(dev, SPI_NOR_CMD_PP, UMA_CODE_CMD_WR_ADR);
	for (int i = 0; i < bytes; i++) {
		cros_flash_npcx_exec_cmd(dev, *src_data, UMA_CODE_CMD_WR_ONLY);
		src_data++;
	}

	/* Chip Select up */
	cros_flash_npcx_cs_level(dev, 1);
}

static int cros_flash_npcx_program_bytes(const struct device *dev,
					 uint32_t offset, uint32_t bytes,
					 const uint8_t *src_data)
{
	int write_size;
	int ret = 0;

	while (bytes > 0) {
		/* Write length can not go beyond the end of the flash page */
		write_size = MIN(bytes,
				 CONFIG_FLASH_WRITE_IDEAL_SIZE -
					 (offset &
					  (CONFIG_FLASH_WRITE_IDEAL_SIZE - 1)));

		/* Enable write */
		ret = cros_flash_npcx_set_write_enable(dev);
		if (ret != 0)
			return ret;

		/* Executr UMA burst write transaction */
		cros_flash_npcx_burst_write(dev, offset, write_size, src_data);

		/* Wait write completed */
		ret = cros_flash_npcx_wait_ready(dev);
		if (ret != 0)
			return ret;

		src_data += write_size;
		offset += write_size;
		bytes -= write_size;
	}

	return ret;
}

/* cros ec flash api functions */
static int cros_flash_npcx_init(const struct device *dev)
{
	const struct cros_flash_npcx_config *const config = DRV_CONFIG(dev);
	struct cros_flash_npcx_data *data = DRV_DATA(dev);

	/* initialize mutux for flash interface controller */
	k_sem_init(&data->lock_sem, 1, 1);

	/* Configure pin-mux for FIU device */
	npcx_pinctrl_mux_configure(config->alts_list, config->alts_size, 1);

	return 0;
}

static int cros_flash_npcx_read(const struct device *dev, int offset, int size,
				char *dst_data)
{
	int ret = 0;

	/* Unlock flash interface device during reading flash */
	cros_flash_npcx_mutex_lock(dev);

	/* Chip Select down */
	cros_flash_npcx_cs_level(dev, 0);

	/* Set read address */
	cros_flash_npcx_set_address(dev, offset);
	/* Start with fast read command (skip one dummy byte) */
	cros_flash_npcx_exec_cmd(dev, SPI_NOR_CMD_FAST_READ,
				 UMA_CODE_CMD_ADR_WR_BYTE(1));
	/* Execute burst read */
	cros_flash_npcx_burst_read(dev, dst_data, size);

	/* Chip Select up */
	cros_flash_npcx_cs_level(dev, 1);

	/* Unlock flash interface device */
	cros_flash_npcx_mutex_unlock(dev);

	return ret;
}

static int cros_flash_npcx_write(const struct device *dev, int offset, int size,
				 const char *src_data)
{
	struct cros_flash_npcx_data *const data = DRV_DATA(dev);
	int ret = 0;

	/* Is write protection enabled? */
	if (data->write_protectied) {
		return -EACCES;
	}

	/* Invalid data pointer? */
	if (src_data == 0) {
		return -EINVAL;
	}

	/* Unlock flash interface device during writing flash */
	cros_flash_npcx_mutex_lock(dev);

	while (size > 0) {
		/* First write multiples of 256, then (size % 256) last */
		int write_len =
			((size % CONFIG_FLASH_WRITE_IDEAL_SIZE) == size) ?
				      size :
				      CONFIG_FLASH_WRITE_IDEAL_SIZE;

		ret = cros_flash_npcx_program_bytes(dev, offset, write_len,
						    src_data);
		if (ret != 0)
			break;

		src_data += write_len;
		offset += write_len;
		size -= write_len;
	}

	/* Unlock flash interface device */
	cros_flash_npcx_mutex_unlock(dev);

	return ret;
}

static int cros_flash_npcx_erase(const struct device *dev, int offset, int size)
{
	const struct cros_flash_npcx_config *const config = DRV_CONFIG(dev);
	struct cros_flash_npcx_data *const data = DRV_DATA(dev);
	int ret = 0;

	/* Is write protection enabled? */
	if (data->write_protectied) {
		return -EACCES;
	}
	/* affected region should be within device */
	if (offset < 0 || (offset + size) > config->size) {
		LOG_ERR("Flash erase address or size exceeds expected values. "
			"Addr: 0x%lx size %zu",
			(long)offset, size);
		return -EINVAL;
	}

	/* address must be aligned to erase size */
	if ((offset % CONFIG_FLASH_ERASE_SIZE) != 0) {
		return -EINVAL;
	}

	/* Erase size must be a non-zero multiple of sectors */
	if ((size == 0) || (size % CONFIG_FLASH_ERASE_SIZE) != 0) {
		return -EINVAL;
	}

	/* Unlock flash interface device during erasing flash */
	cros_flash_npcx_mutex_lock(dev);

	/* Alignment has been checked in upper layer */
	for (; size > 0; size -= CONFIG_FLASH_ERASE_SIZE,
			 offset += CONFIG_FLASH_ERASE_SIZE) {

		/* Enable write */
		ret = cros_flash_npcx_set_write_enable(dev);
		if (ret != 0)
			break;

		/* Set erase address */
		cros_flash_npcx_set_address(dev, offset);
		/* Start erasing */
		cros_flash_npcx_exec_cmd(dev, SPI_NOR_CMD_SE, UMA_CODE_CMD_ADR);

		/* Wait erase completed */
		ret = cros_flash_npcx_wait_ready(dev);
		if (ret != 0) {
			break;
		}
	}

	/* Unlock flash interface device */
	cros_flash_npcx_mutex_unlock(dev);

	return ret;
}

static int cros_flash_npcx_get_status_reg(const struct device *dev,
					  char cmd_code, char *data)
{
	int ret = 0;
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	if (data == 0) {
		return -EINVAL;
	}

	/* Lock flash interface device during reading status register */
	cros_flash_npcx_mutex_lock(dev);

	cros_flash_npcx_exec_cmd(dev, cmd_code, UMA_CODE_CMD_RD_BYTE(1));
	*data = inst->UMA_DB0;
	/* Unlock flash interface device */
	cros_flash_npcx_mutex_unlock(dev);

	return ret;
}

static int cros_flash_npcx_set_status_reg(const struct device *dev, char *data)
{
	int ret = 0;
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	/* Lock flash interface device */
	cros_flash_npcx_mutex_lock(dev);
	/* Enable write */
	ret = cros_flash_npcx_set_write_enable(dev);
	if (ret != 0)
		return ret;

	inst->UMA_DB0 = data[0];
	inst->UMA_DB1 = data[1];
	/* Write status register 1/2 */
	cros_flash_npcx_exec_cmd(dev, SPI_NOR_CMD_WRSR,
				 UMA_CODE_CMD_WR_BYTE(2));
	/* Unlock flash interface device */
	cros_flash_npcx_mutex_unlock(dev);

	return ret;
}

static int cros_flash_npcx_write_protection_set(const struct device *dev,
						bool enable)
{
	int ret = 0;

	/* Write protection can be cleared only by core domain reset */
	if (!enable) {
		LOG_ERR("WP can be disabled only via core domain reset ");
		return -ENOTSUP;
	}
	/* Lock flash interface device */
	cros_flash_npcx_mutex_lock(dev);
	ret = npcx_pinctrl_flash_write_protect_set();
	/* Unlock flash interface device */
	cros_flash_npcx_mutex_unlock(dev);

	return ret;
}

static int cros_flash_npcx_write_protection_is_set(const struct device *dev)
{
	return npcx_pinctrl_flash_write_protect_is_set();
}

static int cros_flash_npcx_uma_lock(const struct device *dev, bool enable)
{
	struct fiu_reg *const inst = HAL_INSTANCE(dev);

	if (enable) {
		inst->UMA_ECTS |= BIT(NPCX_UMA_ECTS_UMA_LOCK);
	} else {
		inst->UMA_ECTS &= ~BIT(NPCX_UMA_ECTS_UMA_LOCK);
	}

	return 0;
}

/* cros ec flash driver registration */
static const struct cros_flash_driver_api cros_flash_npcx_driver_api = {
	.init = cros_flash_npcx_init,
	.physical_read = cros_flash_npcx_read,
	.physical_write = cros_flash_npcx_write,
	.physical_erase = cros_flash_npcx_erase,
	.write_protection = cros_flash_npcx_write_protection_set,
	.write_protection_is_set = cros_flash_npcx_write_protection_is_set,
	.get_status_reg = cros_flash_npcx_get_status_reg,
	.set_status_reg = cros_flash_npcx_set_status_reg,
	.uma_lock = cros_flash_npcx_uma_lock,
};

static int flash_npcx_init(const struct device *dev)
{
	const struct cros_flash_npcx_config *const config = DRV_CONFIG(dev);
	const struct device *const clk_dev =
		device_get_binding(NPCX_CLK_CTRL_NAME);

	int ret;

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev,
			       (clock_control_subsys_t *)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on FIU clock fail %d", ret);
		return ret;
	}

	return ret;
}

static const struct npcx_alt cros_flash_alts[] = NPCX_DT_ALT_ITEMS_LIST(0);
static const struct cros_flash_npcx_config cros_flash_cfg = {
	.base = DT_INST_REG_ADDR(0),
	.clk_cfg = NPCX_DT_CLK_CFG_ITEM(0),
	.size = DT_INST_PROP(0, size),
	.alts_size = ARRAY_SIZE(cros_flash_alts),
	.alts_list = cros_flash_alts,
};

static struct cros_flash_npcx_data cros_flash_data;

DEVICE_AND_API_INIT(cros_flash_npcx_0, DT_INST_LABEL(0), flash_npcx_init,
		    &cros_flash_data, &cros_flash_cfg, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &cros_flash_npcx_driver_api);
