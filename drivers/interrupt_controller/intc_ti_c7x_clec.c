/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_c7x_clec

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/arch/c7x/lib_helpers.h>

#include "intc_ti_c7x_clec.h"

static inline void c7x_efclr(uint8_t local_irq)
{
	/* EFCLR is not indexed. The event goes in the value as a bit. */
	z_c7x_write_efclr(BIT64(local_irq & C7X_IRQ_MASK));
}

#define CLEC_NODE DT_INST(0, ti_c7x_clec)

#define CLEC_ROUTE_IDX(i, node)                                                                    \
	{.soc_event_id = DT_IRQ_BY_IDX(node, i, event),                                            \
	 .local_evt_id = DT_IRQ_BY_IDX(node, i, irq),                                              \
	 .priority = DT_IRQ_BY_IDX(node, i, priority),                                             \
	 .is_level = DT_IRQ_BY_IDX(node, i, flags)},

#define CLEC_ROUTE_NODE(node)                                                                      \
	IF_ENABLED(DT_IRQ_HAS_IDX(node, 0),					\
		   (COND_CODE_1(DT_SAME_NODE(DT_IRQ_INTC_BY_IDX(node, 0), CLEC_NODE), \
				(LISTIFY(DT_NUM_IRQS(node), CLEC_ROUTE_IDX, (), node)), \
				())))

static const struct clec_event_map clec_events[] = {DT_FOREACH_STATUS_OKAY_NODE(CLEC_ROUTE_NODE)};

static const struct clec_cfg clec_config_0 = {
	.base = TI_CLEC_BASE_ADDR,
	.rtmap = CLEC_RTMAP_CPU_ALL,
	.events = clec_events,
	.num_events = (uint16_t)ARRAY_SIZE(clec_events),
};

static void clec_event_route(const struct clec_cfg *cfg, const struct clec_event_map *evt)
{
	uintptr_t mrr_addr = clec_mrr_addr(cfg->base, evt->soc_event_id);
	uint32_t mrr_val;

	clec_mrr_wr(mrr_addr + 8U, 1U);

	mrr_val = clec_mrr_rd(mrr_addr);
	if (evt->is_level != 0U) {
		mrr_val |= CLEC_MRR_IS_LVL_BIT;
	} else {
		mrr_val &= ~CLEC_MRR_IS_LVL_BIT;
	}
	clec_mrr_wr(mrr_addr, mrr_val);

	mrr_val = clec_mrr_rd(mrr_addr);
	mrr_val &= ~(CLEC_MRR_S_BIT | CLEC_MRR_ESE_BIT | CLEC_MRR_RTMAP_MASK |
		     CLEC_MRR_EXT_EVTNUM_MASK | CLEC_MRR_C7X_EVTNUM_MASK);
	mrr_val |= CLEC_MRR_ESE_BIT | FIELD_PREP(CLEC_MRR_RTMAP_MASK, cfg->rtmap) |
		   FIELD_PREP(CLEC_MRR_C7X_EVTNUM_MASK, evt->local_evt_id);
	clec_mrr_wr(mrr_addr, mrr_val);

	c7x_efclr(evt->local_evt_id);
}

/* Local events whose devicetree routes are programmed, one bit per event */
static uint64_t clec_routed;

void c7x_clec_irq_enable(unsigned int irq)
{
	const struct clec_cfg *cfg = &clec_config_0;
	uint64_t bit = BIT64(irq & 0x3FU);
	unsigned int key = irq_lock();

	if ((clec_routed & bit) == 0U) {
		for (uint16_t i = 0; i < cfg->num_events; i++) {
			if (cfg->events[i].local_evt_id == irq) {
				clec_event_route(cfg, &cfg->events[i]);
			}
		}
		clec_routed |= bit;
	}

	irq_unlock(key);
}

void c7x_clec_access_init(void)
{
	/* The CLEC takes events 1..510. Store the whole access-control word, so
	 * the secure claim and IS_LVL are cleared rather than kept at POR.
	 */
	const struct clec_cfg *cfg = &clec_config_0;

	for (uint32_t i = 1U; i < 511U; i++) {
		clec_mrr_wr(clec_mrr_addr(cfg->base, i),
			    FIELD_PREP(CLEC_MRR_RTMAP_MASK, CLEC_RTMAP_DISABLE));
	}
}

int c7x_clec_irq_of_event(uint32_t soc_event)
{
	const struct clec_cfg *cfg = &clec_config_0;

	for (uint16_t i = 0; i < cfg->num_events; i++) {
		if (cfg->events[i].soc_event_id == soc_event) {
			return cfg->events[i].local_evt_id;
		}
	}

	return -ENOENT;
}

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);
