/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/c7x/arch.h>
#include <zephyr/tracing/tracing.h>

/* No wait instruction. IDLE does not reliably wake, so the idle loop spins. */
static ALWAYS_INLINE void c7x_idle(unsigned int key)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif
	irq_unlock(key);
#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif
}

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
	c7x_idle(C7X_IRQ_UNLOCKED);
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
	c7x_idle(key);
}
#endif

#define C7X_CYCLES_PER_USEC (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / USEC_PER_SEC)

void arch_busy_wait(uint32_t usec_to_wait)
{
	uint64_t target = (uint64_t)usec_to_wait * C7X_CYCLES_PER_USEC;
	uint64_t start;

	if (usec_to_wait == 0U) {
		return;
	}

	start = read_tsc();
	while ((read_tsc() - start) < target) {
	}
}
