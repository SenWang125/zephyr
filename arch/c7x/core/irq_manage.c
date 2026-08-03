/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include "sw_isr_common.h"
#include <zephyr/sys/printk.h>
#include <zephyr/toolchain.h>
#include <kernel_internal.h>
#include <ksched.h>
#include <kswap.h>
#include <zephyr/tracing/tracing.h>

#include <zephyr/arch/c7x/arch.h>
#include <zephyr/arch/c7x/irq.h>

BUILD_ASSERT(CONFIG_NUM_IRQS == C7X_NUM_IRQS, "the ISR table must cover every IRQ");

void arch_irq_enable(unsigned int irq)
{
	z_c7x_write_eeset(UINT64_C(1) << (irq & C7X_IRQ_MASK));
}

void arch_irq_disable(unsigned int irq)
{
	z_c7x_write_eeclr(UINT64_C(1) << (irq & C7X_IRQ_MASK));
}

void z_c7x_irq_priority_set(unsigned int irq, unsigned int prio)
{
	uint64_t p = CLAMP(prio, C7X_EPRI_PRIO_MIN, C7X_EPRI_PRIO_MAX);
	unsigned int key = z_c7x_irq_lock();

	z_c7x_write_epri(irq & C7X_IRQ_MASK, p << C7X_EPRI_PRIO_SHIFT);
	z_c7x_irq_unlock(key);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return (int)((z_c7x_read_eer() >> (irq & C7X_IRQ_MASK)) & UINT64_C(1));
}

void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);

	z_c7x_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

void z_c7x_isr_handler(unsigned int evt_id)
{
	const struct _isr_table_entry *table = _sw_isr_table;

	/* an interrupt can leave an NLC reload pending, so clear it before any subsequent task */
	z_c7x_nlc_refresh();

	_current_cpu->nested++;

#ifdef CONFIG_SCHED_THREAD_USAGE
	z_sched_usage_stop();
#endif
#ifdef CONFIG_TRACING
	sys_trace_isr_enter();
#endif

	table[evt_id].isr(table[evt_id].arg);

#ifdef CONFIG_TRACING
	sys_trace_isr_exit();
#endif
#ifdef CONFIG_STACK_SENTINEL
	z_check_stack_sentinel();
#endif

	_current_cpu->nested--;

#ifdef CONFIG_SCHED_THREAD_USAGE
	z_sched_usage_switch(_current_cpu->current);
#endif
}

void z_c7x_isr_exit_resched(void)
{
	/* No lock needed since events remained disabled throughout this func */
	__ASSERT_NO_MSG(!arch_cpu_irqs_are_enabled());

	if (_current_cpu->nested == 0) {
		struct k_thread *old = _current;
		void *next = z_sched_next_handle(old);

		if (next != NULL) {
			/* a switch skips RETE, therefore restore COP level to
			 * default task mode for the next thread.
			 */
			z_c7x_set_cop(C7X_TSR_COP_TASK_MODE);
			arch_switch(next, &old->switch_handle);
		}
	}
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter), const void *parameter,
			     uint32_t flags)
{
	ARG_UNUSED(flags);

	if (irq >= CONFIG_NUM_IRQS) {
		return -EINVAL;
	}
	z_isr_install(irq, routine, parameter);
	z_c7x_irq_priority_set(irq, priority);

	return irq;
}

int arch_irq_disconnect_dynamic(unsigned int irq, unsigned int priority,
				void (*routine)(const void *parameter), const void *parameter,
				uint32_t flags)
{
	ARG_UNUSED(priority);
	ARG_UNUSED(flags);

	if (irq >= CONFIG_NUM_IRQS) {
		return -EINVAL;
	}

#ifdef CONFIG_SHARED_INTERRUPTS
	int ret = z_isr_uninstall(irq, routine, parameter);

	/* the line is disabled once its last client is gone */
	if (_sw_isr_table[z_get_sw_isr_table_idx(irq)].isr == z_irq_spurious) {
		arch_irq_disable(irq);
	}

	return ret;
#else
	ARG_UNUSED(routine);
	ARG_UNUSED(parameter);

	arch_irq_disable(irq);
	z_isr_install(irq, z_irq_spurious, NULL);

	return 0;
#endif
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
