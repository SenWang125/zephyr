/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief C7x interrupt interface
 */

#ifndef ZEPHYR_INCLUDE_ARCH_C7X_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_C7X_IRQ_H_

#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sw_isr_table.h>

#include <zephyr/arch/c7x/cpu.h>
#include <zephyr/arch/c7x/lib_helpers.h>

#ifdef __cplusplus
extern "C" {
#endif

#define C7X_NUM_IRQS            64U
#define C7X_IRQ_MASK            (C7X_NUM_IRQS - 1U)

/* prio runs 1 (lowest) to 7 (highest), default is 6 */
#define ARCH_IRQ_CONNECT(irq_n, prio, isr_fn, isr_arg, flags)                                      \
	{                                                                                          \
		Z_ISR_DECLARE((irq_n), 0, isr_fn, (isr_arg));                                      \
		z_c7x_irq_priority_set((irq_n), (prio));                                           \
	}

extern void arch_irq_enable(unsigned int irq);
extern void arch_irq_disable(unsigned int irq);
extern int arch_irq_is_enabled(unsigned int irq);

static ALWAYS_INLINE void z_c7x_irq_priority_set(unsigned int irq, unsigned int prio)
{
	uint64_t idx = (uint64_t)(irq & C7X_IRQ_MASK);
	uint64_t p = (prio > C7X_EPRI_PRIO_MAX) ? C7X_EPRI_PRIO_MAX :
		     (prio < C7X_EPRI_PRIO_MIN) ? C7X_EPRI_PRIO_MIN : (uint64_t)prio;
	uint64_t epri_val = p << C7X_EPRI_PRIO_SHIFT;

	z_c7x_write_epri((unsigned int)idx, epri_val);
}

/*
 * All 64 events share one vector, therefore a direct ISR still
 * runs through the common entry and ISR table.
 */
#define ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p)                                 \
	{                                                                                          \
		Z_ISR_DECLARE_DIRECT((irq_p), ISR_FLAG_DIRECT, isr_p);                             \
		z_c7x_irq_priority_set((irq_p), (priority_p));                                     \
	}

#define ARCH_ISR_DIRECT_HEADER()	do { } while (false)
#define ARCH_ISR_DIRECT_FOOTER(swap)	do { (void)(swap); } while (false)
#define ARCH_ISR_DIRECT_PM()		do { } while (false)

#define ARCH_ISR_DIRECT_DECLARE(name)					\
	static inline int name##_body(void);				\
	void name(const void *unused)					\
	{								\
		ARG_UNUSED(unused);					\
		ISR_DIRECT_HEADER();					\
		(void)name##_body();					\
		ISR_DIRECT_FOOTER(0);					\
	}								\
	static inline int name##_body(void)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_C7X_IRQ_H_ */
