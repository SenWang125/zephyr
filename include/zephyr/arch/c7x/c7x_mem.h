/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief C7x memory flags for k_mem_map_phys_bare()
 *
 * K_MEM_CACHE_WB is MAIR7, WT is MAIR6, NONE is MAIR0,
 * and the flags below name the rest.
 */
#ifndef ZEPHYR_INCLUDE_ARCH_C7X_C7X_MEM_H_
#define ZEPHYR_INCLUDE_ARCH_C7X_C7X_MEM_H_

/** Device memory, nGnRnE (MAIR0), carries K_MEM_CACHE_NONE value */
#define K_MEM_C7X_DEVICE_nGnRnE	K_MEM_CACHE_NONE

/** Device memory, nGnRE (MAIR1) */
#define K_MEM_C7X_DEVICE_nGnRE	3

/** Device memory, GRE (MAIR3) */
#define K_MEM_C7X_DEVICE_GRE	4

/** Normal memory, non-cacheable (MAIR4) */
#define K_MEM_C7X_NORMAL_NC	5

/** Device memory, nGRE (MAIR2) */
#define K_MEM_C7X_DEVICE_nGRE	6

/** Normal memory, inner write-back, outer non-cacheable (MAIR5) */
#define K_MEM_C7X_NORMAL_INNER_WB	7

#endif /* ZEPHYR_INCLUDE_ARCH_C7X_C7X_MEM_H_ */
