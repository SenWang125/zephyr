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
#include <c7x_frame.h>

#define C7X_CFRAME_BYTES        (C7X_CFRAME_SLOTS * sizeof(uint64_t))

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	uint64_t *frame;
	uintptr_t tcsp = (uintptr_t)stack;

	/* Carve the context frame just below the aligned stack top,
	 * with the EABI 16-byte free/reserve area in it.
	 */
	stack_ptr = (char *)((((uintptr_t)stack_ptr - C7X_EABI_FREE_AREA) -
			      C7X_CFRAME_BYTES) & ~(uintptr_t)(ARCH_STACK_PTR_ALIGN - 1U));
	frame = (uint64_t *)stack_ptr;

	/* Check stack object TCSP record integrity */
	if ((tcsp & (C7X_CONTEXT_SAVE_SIZE - 1U)) != 0U) {
		k_panic();
	}

	frame[C7X_CFRAME_SLOT_TCSP] = (uint64_t)tcsp;

	frame[C7X_CFRAME_SLOT_RESERVED0] = 0U;
	frame[C7X_CFRAME_SLOT_RESERVED1] = 0U;

	frame[C7X_CFRAME_SLOT_A8]  = (uint64_t)(uintptr_t)entry;
	frame[C7X_CFRAME_SLOT_A9]  = (uint64_t)(uintptr_t)p1;
	frame[C7X_CFRAME_SLOT_A10] = (uint64_t)(uintptr_t)p2;
	frame[C7X_CFRAME_SLOT_A11] = (uint64_t)(uintptr_t)p3;

	frame[C7X_CFRAME_SLOT_A12] = 0U;
	frame[C7X_CFRAME_SLOT_A13] = 0U;
	frame[C7X_CFRAME_SLOT_A14] = 0U;
	frame[C7X_CFRAME_SLOT_A15] = (uint64_t)(uintptr_t)frame + C7X_CFRAME_BYTES;

	frame[C7X_CFRAME_SLOT_B14] = 0U;
	frame[C7X_CFRAME_SLOT_B15] = 0U;

	frame[C7X_CFRAME_SLOT_TSR] = z_c7x_read_tsr();

	frame[C7X_CFRAME_SLOT_GLUE_RP] = (uint64_t)(uintptr_t)z_c7x_thread_start;
	frame[C7X_CFRAME_SLOT_RP] = (uint64_t)(uintptr_t)z_c7x_thread_start;

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
