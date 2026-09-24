/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief C7x specific kernel interface header
 */

#ifndef ZEPHYR_INCLUDE_ARCH_C7X_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_C7X_ARCH_H_

#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/c7x/cpu.h>
#include <zephyr/arch/c7x/thread.h>
#include <zephyr/arch/c7x/irq.h>
#include <zephyr/arch/c7x/lib_helpers.h>
#include <zephyr/arch/c7x/exception.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The C7x EABI requires 8-byte stack pointer alignment. */
#define ARCH_STACK_PTR_ALIGN            8U

/* Reserve the worst-case ISR frame, saved on the interrupted SP. */
#define C7X_ISR_FRAME_RESERVE (C7X_EABI_FREE_AREA + C7X_ISR_FRAME_WORST_CASE)

/*
 * A thread's TCSP record is the first C7X_CONTEXT_SAVE_SIZE bytes of its stack
 * object, and the object is aligned to C7X_CONTEXT_SAVE_SIZE. The ISR frame
 * reserve sits above TCSP record.
 */
#define ARCH_THREAD_STACK_RESERVED        (C7X_CONTEXT_SAVE_SIZE + C7X_ISR_FRAME_RESERVE)
#define ARCH_THREAD_STACK_OBJ_ALIGN(size) C7X_CONTEXT_SAVE_SIZE

/* Kernel stacks are no different from thread stacks, as userspace is not
 * supported. */
#define ARCH_KERNEL_STACK_RESERVED      ARCH_THREAD_STACK_RESERVED
#define ARCH_KERNEL_STACK_OBJ_ALIGN     C7X_CONTEXT_SAVE_SIZE

/* Key passed to arch_irq_unlock() when the state is already unlocked. */
#define C7X_IRQ_UNLOCKED	1U

extern unsigned int z_c7x_irq_lock(void);
extern void         z_c7x_irq_unlock(unsigned int key);

void z_c7x_set_cop(uint32_t cop);

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	return z_c7x_irq_lock();
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	z_c7x_irq_unlock(key);
}

static inline void arch_nop(void)
{
	__asm(" NOP");
}

static inline uint32_t arch_k_cycle_get_32(void)
{
	return (uint32_t)z_c7x_read_tsc();
}

static inline uint64_t arch_k_cycle_get_64(void)
{
	return z_c7x_read_tsc();
}

static inline bool arch_irq_unlocked(unsigned int key)
{
	return key != 0U;
}

static inline bool arch_cpu_irqs_are_enabled(void)
{
	return (z_c7x_read_tsr() & C7X_TSR_GEE) != 0U;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_C7X_ARCH_H_ */
