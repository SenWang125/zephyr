/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 *
 *  RTMAP field values:
 *    DISABLE = (0x01 << 0) = 0x01: disable routing
 *    SYS     = (0x01 << 1) = 0x02: route to SoC interrupt router
 *    CPU_0   = (0x00 << 2) = 0x00: CPU 0 (ARM)
 *    CPU_4   = (0x04 << 2) = 0x10: CPU 4 (C7x)
 *    CPU_ALL = (0x0F << 2) = 0x3C. All CPUs
 *
 *  AM62D C7x_0 uses CPU_ALL.
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_C7X_CLEC_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_C7X_CLEC_H_

#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/intc_ti_c7x_clec.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TI_CLEC_BASE_ADDR           DT_REG_ADDR(DT_INST(0, ti_c7x_clec))

#define CLEC_MRR_BASE_OFFSET        0x00080000U
#define CLEC_MRR_STRIDE             0x100U

static inline uintptr_t clec_mrr_addr(uintptr_t base, uint32_t event_id)
{
	return base + CLEC_MRR_BASE_OFFSET + (event_id * CLEC_MRR_STRIDE);
}

#define CLEC_MRR_S_BIT              BIT(31)
#define CLEC_MRR_ESE_BIT            BIT(30)
#define CLEC_MRR_IS_LVL_BIT         BIT(24)
#define CLEC_MRR_RTMAP_SHIFT        16U
#define CLEC_MRR_RTMAP_MASK         (0x3FU << CLEC_MRR_RTMAP_SHIFT)
#define CLEC_MRR_EXT_EVTNUM_SHIFT   8U
#define CLEC_MRR_EXT_EVTNUM_MASK    (0xFFU << CLEC_MRR_EXT_EVTNUM_SHIFT)
#define CLEC_MRR_C7X_EVTNUM_MASK    0x3FU

#define CLEC_RTMAP_DISABLE          0x01U
#define CLEC_RTMAP_CPU_ALL          0x3CU   /* all CPUs */

struct clec_event_map {
	uint16_t soc_event_id;
	uint8_t  local_evt_id;
	uint8_t  priority;
	uint8_t  is_level;
};

struct clec_cfg {
	uintptr_t             base;
	uint8_t               rtmap;
	const struct clec_event_map *events;
	uint16_t              num_events;
};

static inline uint32_t clec_mrr_rd(uintptr_t addr)
{
	return sys_read32(addr);
}

static inline void clec_mrr_wr(uintptr_t addr, uint32_t val)
{
	sys_write32(val, addr);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_C7X_CLEC_H_ */
