/*
 *  Copyright (c) 2026 Texas Instruments Incorporated
 *  SPDX-License-Identifier: Apache-2.0
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

/*
 *  Matches portBYTE_ALIGNMENT in the MCU+ SDK FreeRTOS C7x port
 *  (kernel/freertos/portable/TI_CGT/DSP_C75X/portmacro.h:102).
 */
#define ARCH_STACK_PTR_ALIGN            16U

/* the ISR save frame lands on the interrupted thread's stack */
#define C7X_ISR_FRAME_RESERVE (C7X_EABI_FREE_AREA + C7X_ISR_FRAME_WORST_CASE)

/* A thread's TCSP record is the first 0x2000 of its stack object, as the SDK
 * carves it from the task stack; TCSP bits 12:0 read as zero, so the object is 0x2000-aligned.
 */
#define ARCH_THREAD_STACK_RESERVED        (C7X_CONTEXT_SAVE_SIZE + C7X_ISR_FRAME_RESERVE)
#define ARCH_THREAD_STACK_OBJ_ALIGN(size) C7X_CONTEXT_SAVE_SIZE

/* The frame lands on whichever stack is interrupted, and the idle thread's is a
 * K_KERNEL_STACK, so kernel stacks need the same reserve as thread stacks.
 */
#define ARCH_KERNEL_STACK_RESERVED      ARCH_THREAD_STACK_RESERVED
#define ARCH_KERNEL_STACK_OBJ_ALIGN     C7X_CONTEXT_SAVE_SIZE

/* the lock key of a context that has events enabled */
#define C7X_IRQ_UNLOCKED	1U

extern unsigned int c7x_irq_lock(void);
extern void         c7x_irq_unlock(unsigned int key);

void c7x_set_cop(uint32_t cop);

static inline unsigned int arch_irq_lock(void)
{
	return c7x_irq_lock();
}

static inline void arch_irq_unlock(unsigned int key)
{
	c7x_irq_unlock(key);
}

static inline void arch_nop(void)
{
	__asm(" NOP");
}

static inline uint32_t arch_k_cycle_get_32(void)
{
	return (uint32_t)read_tsc();
}

static inline uint64_t arch_k_cycle_get_64(void)
{
	return read_tsc();
}

static inline bool arch_irq_unlocked(unsigned int key)
{
	return key != 0U;
}

static inline bool arch_cpu_irqs_are_enabled(void)
{
	return (read_tsr() & C7X_TSR_GEE) != 0U;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_C7X_ARCH_H_ */
