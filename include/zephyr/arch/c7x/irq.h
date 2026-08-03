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

#define C7X_NUM_IRQS 64U
#define C7X_IRQ_MASK (C7X_NUM_IRQS - 1U)

/* prio runs 1 (highest) to 7 (lowest), default is 6 */
#define ARCH_IRQ_CONNECT(irq_n, prio, isr_fn, isr_arg, flags)                                      \
	{                                                                                          \
		Z_ISR_DECLARE((irq_n), 0, isr_fn, (isr_arg));                                      \
		z_c7x_irq_priority_set((irq_n), (prio));                                           \
	}

void arch_irq_enable(unsigned int irq);
void arch_irq_disable(unsigned int irq);
int arch_irq_is_enabled(unsigned int irq);

void z_c7x_irq_priority_set(unsigned int irq, unsigned int prio);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_C7X_IRQ_H_ */
