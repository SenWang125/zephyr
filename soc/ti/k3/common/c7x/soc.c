/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/drivers/interrupt_controller/intc_ti_c7x_clec.h>

#include "ctrl_partitions.h"
#include "soc.h"

#define WKUP_CONF DT_NODELABEL(wkup_conf)

void soc_prep_hook(void)
{
#ifdef CONFIG_TI_C7X_CLEC
	c7x_clec_access_init();
#endif
}

void soc_early_init_hook(void)
{
	mm_reg_t wkup_conf;
	mm_reg_t clkgate;

	k3_unlock_all_ctrl_partitions();

	device_map(&wkup_conf, DT_REG_ADDR(WKUP_CONF), DT_REG_SIZE(WKUP_CONF), K_MEM_CACHE_NONE);
	clkgate = wkup_conf + TI_K3_WKUP_CLKGATE_CTRL1;
	/* Keep the C7x clock running while the core is idle */
	sys_write32(sys_read32(clkgate) | TI_K3_WKUP_CLKGATE_CTRL1_C7X_NOGATE, clkgate);
	IF_ENABLED(CONFIG_MMU,
		   (k_mem_unmap_phys_bare((uint8_t *)wkup_conf, DT_REG_SIZE(WKUP_CONF));))
}
