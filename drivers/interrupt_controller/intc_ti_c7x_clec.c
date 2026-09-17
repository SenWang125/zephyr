/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_c7x_clec

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/arch/c7x/lib_helpers.h>

#include "intc_ti_c7x_clec.h"

static inline void c7x_epri_set(uint8_t local_irq, uint8_t priority)
{
	/* EPRI holds the priority in bits 7:5. Clamp rather than mask, as
	 * c7x_irq_priority_set() does. Masking turns 0 or 8 into EPRI 0, highest.
	 */
	unsigned int p = (priority > 7U) ? 7U : (priority < 1U) ? 1U : priority;

	write_epri((unsigned int)local_irq, p << 5U);
}

static inline void c7x_efclr(uint8_t local_irq)
{
	/* EFCLR is not indexed. The event goes in the VALUE as a bit, unlike
		 * __EPRI above.
		 */
	write_efclr(UINT64_C(1) << (local_irq & 0x3FU));
}

#define CLEC_NODE DT_INST(0, ti_c7x_clec)

#define CLEC_ROUTE_IDX(i, node)							\
	{ .soc_event_id = DT_IRQ_BY_IDX(node, i, event),			\
	  .local_evt_id = DT_IRQ_BY_IDX(node, i, irq),				\
	  .priority = DT_IRQ_BY_IDX(node, i, priority),				\
	  .is_level = DT_IRQ_BY_IDX(node, i, flags) },

#define CLEC_ROUTE_NODE(node)							\
	IF_ENABLED(DT_IRQ_HAS_IDX(node, 0),					\
		   (COND_CODE_1(DT_SAME_NODE(DT_IRQ_INTC_BY_IDX(node, 0), CLEC_NODE), \
				(LISTIFY(DT_NUM_IRQS(node), CLEC_ROUTE_IDX, (), node)), \
				())))

static const struct clec_event_map clec_events[] = {
	DT_FOREACH_NODE(CLEC_ROUTE_NODE)
};

const struct clec_cfg clec_config_0 = {
	.base       = TI_CLEC_BASE_ADDR,
	.rtmap      = CLEC_RTMAP_CPU_ALL,
	.events     = clec_events,
	.num_events = (uint16_t)ARRAY_SIZE(clec_events),
};

static const struct clec_event_map *clec_find_by_local_id(
	const struct clec_cfg *cfg, uint8_t local_id)
{
	for (uint16_t i = 0; i < cfg->num_events; i++) {
		if (cfg->events[i].local_evt_id == local_id) {
			return &cfg->events[i];
		}
	}
	return NULL;
}

/* A system timer routed through this CLEC keeps its route across the disable pass. */
#define CLEC_TIMER_EVENT(node)							\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, ti_am654_timer),			\
		   (IF_ENABLED(DT_IRQ_HAS_IDX(node, 0),				\
			       (COND_CODE_1(DT_SAME_NODE(DT_IRQ_INTC_BY_IDX(node, 0), \
							  DT_DRV_INST(0)),	\
					    (DT_IRQ(node, event),), ())))))

static const uint16_t clec_kept_routes[] = {
	DT_FOREACH_NODE(CLEC_TIMER_EVENT)
};

static bool clec_route_kept(uint32_t soc_event)
{
	for (size_t i = 0; i < ARRAY_SIZE(clec_kept_routes); i++) {
		if (clec_kept_routes[i] == soc_event) {
			return true;
		}
	}
	return false;
}

static int clec_init(const struct device *dev)
{
	/* The CLEC takes events 1..510. Store the whole access-control word, so
		 * IS_LVL is cleared rather than kept at POR.
		 */
	const struct clec_cfg *cfg = &clec_config_0;

	ARG_UNUSED(dev);

	for (uint32_t i = 1U; i < 511U; i++) {
		if (clec_route_kept(i)) {
			continue;
		}
		clec_mrr_wr(clec_mrr_addr(cfg->base, i),
			    (uint32_t)CLEC_RTMAP_DISABLE << CLEC_MRR_RTMAP_SHIFT);
	}

	return 0;
}

__attribute__((noinline))
void c7x_clec_irq_enable(unsigned int local_irq)
{
	volatile unsigned int saved_irq = local_irq;

	const struct clec_cfg *cfg = &clec_config_0;

	unsigned int the_irq = saved_irq;
	const struct clec_event_map *evt;
	uint32_t mrr_val;
	uintptr_t mrr_addr;
	uintptr_t ecr_addr;

	evt = clec_find_by_local_id(cfg, (uint8_t)the_irq);
	if (evt == NULL) {
		return;
	}

	mrr_addr = clec_mrr_addr(cfg->base, evt->soc_event_id);
	ecr_addr = mrr_addr + 8U;

	clec_mrr_wr(ecr_addr, 1U);

	mrr_val = clec_mrr_rd(mrr_addr);
	if (evt->is_level) {
		mrr_val |= CLEC_MRR_IS_LVL_BIT;
	} else {
		mrr_val &= ~CLEC_MRR_IS_LVL_BIT;
	}
	clec_mrr_wr(mrr_addr, mrr_val);

	mrr_val = clec_mrr_rd(mrr_addr);
	mrr_val &= ~(CLEC_MRR_S_BIT
			   | CLEC_MRR_ESE_BIT
			   | CLEC_MRR_RTMAP_MASK
			   | CLEC_MRR_EXT_EVTNUM_MASK
			   | CLEC_MRR_C7X_EVTNUM_MASK);
	mrr_val |= CLEC_MRR_ESE_BIT
			 | ((uint32_t)(cfg->rtmap & 0x3FU) << CLEC_MRR_RTMAP_SHIFT)
			 | ((uint32_t)(evt->local_evt_id & 0x3FU));
	clec_mrr_wr(mrr_addr, mrr_val);

	c7x_epri_set(evt->local_evt_id, evt->priority);
	c7x_efclr(evt->local_evt_id);
}

/*
 *  Route a runtime-chosen soc_event to a C7x local event. The UDMA and McASP
 *  drivers compute their completion mapping at runtime, so those events are not
 *  in the devicetree-generated clec_events[]. Programs a new event only and
 *  leaves the pre-claimed mailbox routes alone.
 */
__attribute__((noinline))
void c7x_clec_route_program(uint32_t soc_event, uint32_t c7x_evt,
			    unsigned int is_level, unsigned int priority)
{
	volatile uint32_t v_soc = soc_event;
	volatile uint32_t v_evt = c7x_evt;
	const struct clec_cfg *cfg = &clec_config_0;
	uint32_t mrr_val;
	uintptr_t mrr_addr;
	uintptr_t ecr_addr;

	mrr_addr = clec_mrr_addr(cfg->base, (uint32_t)v_soc);
	ecr_addr = mrr_addr + 8U;

	clec_mrr_wr(ecr_addr, 1U);

	mrr_val = clec_mrr_rd(mrr_addr);
	if (is_level != 0U) {
		mrr_val |= CLEC_MRR_IS_LVL_BIT;
	} else {
		mrr_val &= ~CLEC_MRR_IS_LVL_BIT;
	}
	clec_mrr_wr(mrr_addr, mrr_val);

	mrr_val = clec_mrr_rd(mrr_addr);
	mrr_val &= ~(CLEC_MRR_S_BIT
			   | CLEC_MRR_ESE_BIT
			   | CLEC_MRR_RTMAP_MASK
			   | CLEC_MRR_EXT_EVTNUM_MASK
			   | CLEC_MRR_C7X_EVTNUM_MASK);
	mrr_val |= CLEC_MRR_ESE_BIT
			 | ((uint32_t)(cfg->rtmap & 0x3FU) << CLEC_MRR_RTMAP_SHIFT)
			 | ((uint32_t)(v_evt & 0x3FU));
	clec_mrr_wr(mrr_addr, mrr_val);

	c7x_epri_set((uint8_t)v_evt, (uint8_t)priority);
	c7x_efclr((uint8_t)v_evt);
}

DEVICE_DT_INST_DEFINE(0, clec_init, NULL, NULL, NULL, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,
		      NULL);
