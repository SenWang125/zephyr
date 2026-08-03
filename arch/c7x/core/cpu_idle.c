/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/c7x/arch.h>
#include <zephyr/tracing/tracing.h>

/* C7x IDLE instruction depends on L1/SE/CMMU status and won't return
 * whilst any is open, so wait for an enabled event flag with events locked.
 */
static ALWAYS_INLINE void z_c7x_cpu_idle(unsigned int key)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif
	while ((z_c7x_read_efr() & z_c7x_read_eer()) == 0U) {
	}
	irq_unlock(key);
#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif
}

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
	(void)arch_irq_lock();
	z_c7x_cpu_idle(C7X_IRQ_UNLOCKED);
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
	z_c7x_cpu_idle(key);
}
#endif

void arch_busy_wait(uint32_t usec_to_wait)
{
	uint64_t start = k_cycle_get_64();
	uint64_t cycles_to_wait = k_us_to_cyc_ceil64(usec_to_wait);

	while ((k_cycle_get_64() - start) < cycles_to_wait) {
	}
}
