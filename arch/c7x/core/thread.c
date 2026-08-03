/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <ksched.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <errno.h>
#include <string.h>

#include <zephyr/arch/c7x/arch.h>
#include <zephyr/arch/c7x/thread.h>
#include <c7x.h>
#include <kernel_arch_data.h>

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack, char *stack_ptr,
		     k_thread_entry_t entry, void *p1, void *p2, void *p3)
{
	struct c7x_switch_frame *frame;
	uintptr_t tcsp = (uintptr_t)stack;

	/* Carve the context frame just below the aligned stack top,
	 * with the EABI 16-byte free/reserve area in it.
	 */
	stack_ptr = (char *)((((uintptr_t)stack_ptr - C7X_EABI_FREE_AREA) - sizeof(*frame)) &
			     ~(uintptr_t)(ARCH_STACK_PTR_ALIGN - 1U));
	frame = (struct c7x_switch_frame *)stack_ptr;

	/* Check integrity of stack's TCSP record */
	if (!IS_ALIGNED(tcsp, C7X_CONTEXT_SAVE_SIZE)) {
		k_panic();
	}

	frame->tcsp = (uint64_t)tcsp;

	frame->reserved[0] = 0U;
	frame->reserved[1] = 0U;

	frame->a8 = (uint64_t)(uintptr_t)entry;
	frame->a9 = (uint64_t)(uintptr_t)p1;
	frame->a10 = (uint64_t)(uintptr_t)p2;
	frame->a11 = (uint64_t)(uintptr_t)p3;

	frame->a12 = 0U;
	frame->a13 = 0U;
	frame->a14 = 0U;
	frame->a15 = (uint64_t)(uintptr_t)(frame + 1);

	frame->b14 = 0U;
	frame->b15 = 0U;

	frame->tsr = z_c7x_read_tsr();

	frame->glue_rp = (uint64_t)(uintptr_t)z_c7x_thread_start;
	frame->rp = (uint64_t)(uintptr_t)z_c7x_thread_start;

	thread->switch_handle = frame;
}

void z_c7x_thread_entry_wrapper(k_thread_entry_t entry, void *p1, void *p2, void *p3)
{
	arch_irq_unlock(C7X_IRQ_UNLOCKED);
	z_thread_entry(entry, p1, p2, p3);
}

int arch_coprocessors_disable(struct k_thread *thread)
{
	ARG_UNUSED(thread);

	return -ENOTSUP;
}
