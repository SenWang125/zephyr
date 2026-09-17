/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/sys/printk.h>
#include <zephyr/toolchain.h>
#include <kernel_internal.h>
#include <ksched.h>
#include <kswap.h>
#include <zephyr/tracing/tracing.h>

#include <zephyr/arch/c7x/arch.h>
#include <zephyr/arch/c7x/irq.h>

void arch_irq_enable(unsigned int irq)
{
	write_eeset(UINT64_C(1) << (irq & C7X_IRQ_MASK));
}

void arch_irq_disable(unsigned int irq)
{
	write_eeclr(UINT64_C(1) << (irq & C7X_IRQ_MASK));
}

int arch_irq_is_enabled(unsigned int irq)
{
	return (int)((read_eer() >> (irq & C7X_IRQ_MASK)) & UINT64_C(1));
}

BUILD_ASSERT(CONFIG_NUM_IRQS == C7X_NUM_IRQS, "the ISR table must cover every event line");

void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);

	c7x_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

void c7x_isr_handler(unsigned int evt_id)
{
	_current_cpu->nested++;

	const struct _isr_table_entry *table = _sw_isr_table;

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


void c7x_isr_exit_resched(void)
{
	if (_current_cpu->nested == 0) {
		struct k_thread *old = _current;
		unsigned int key = c7x_irq_lock();
		void *next = z_sched_next_handle(old);

		if (next != NULL) {
			arch_switch(next, &old->switch_handle);
		}
		c7x_irq_unlock(key);
		/* Padding adjacent instr with NOP, as per TI DOCU-470:
		 * NLC state corrupts if an event lands during NLCINIT
		 */
		__asm__(" MVKU32 .L1 0x1, A0\n NOP 4\n"
			" NLCINIT .S1 A0, 0x1, 0\n NOP 4\n");
	}
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
/* Installs the ISR and sets the priority, as ARCH_IRQ_CONNECT does. 
 * Note the driver still has to acquire/route the event via c7x_clec_irq_enable().
 */
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter),
			     const void *parameter, uint32_t flags)
{
	ARG_UNUSED(flags);

	if (irq >= CONFIG_NUM_IRQS) {
		return -EINVAL;
	}
	z_isr_install(irq, routine, parameter);
	c7x_irq_priority_set(irq, priority);

	return irq;
}

/* Restores the default entry, per arch_interface.h. */
int arch_irq_disconnect_dynamic(unsigned int irq, unsigned int priority,
				void (*routine)(const void *parameter),
				const void *parameter, uint32_t flags)
{
	ARG_UNUSED(priority);
	ARG_UNUSED(routine);
	ARG_UNUSED(parameter);
	ARG_UNUSED(flags);

	if (irq >= CONFIG_NUM_IRQS) {
		return -EINVAL;
	}
	arch_irq_disable(irq);
	z_isr_install(irq, z_irq_spurious, NULL);

	return 0;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
