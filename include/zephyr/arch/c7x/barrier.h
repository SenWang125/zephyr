/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief C7x memory barrier operations
 */

#ifndef ZEPHYR_INCLUDE_ARCH_C7X_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_C7X_BARRIER_H_

#ifndef ZEPHYR_INCLUDE_SYS_BARRIER_H_
#error Please include <zephyr/sys/barrier.h>
#endif

#include <zephyr/toolchain.h>

#include <c7x.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MFENCE x stalls until pending loads and stores carrying tag x is complete, and
 * C7x fetches through the same MFENCE-ordered path, regardless of instr or data.
 */
static ALWAYS_INLINE void z_barrier_dmem_fence_full(void)
{
	__memory_fence(__MFENCE_ALL_COLORS);
}

static ALWAYS_INLINE void z_barrier_dsync_fence_full(void)
{
	__memory_fence(__MFENCE_ALL_COLORS);
}

static ALWAYS_INLINE void z_barrier_isync_fence_full(void)
{
	__memory_fence(__MFENCE_ALL_COLORS);
}

static ALWAYS_INLINE void z_barrier_sync_synchronize(void)
{
	__memory_fence(__MFENCE_ALL_COLORS);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_C7X_BARRIER_H_ */
