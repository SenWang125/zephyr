/*
 *  Copyright (c) 2026 Texas Instruments Incorporated
 *  SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief C7x L1 data cache operations
 */
#ifndef ZEPHYR_INCLUDE_ARCH_C7X_CACHE_H_
#define ZEPHYR_INCLUDE_ARCH_C7X_CACHE_H_

#include <stddef.h>
#include <stdint.h>

/** The L1DWBINV operand that writes back and invalidates the whole cache */
#define C7X_L1D_WBINV_ALL	1U

void c7x_l1d_wbinv(uint64_t val);
uint64_t c7x_l1dcfg_get(void);
void c7x_l1dcfg_set(uint64_t cfg);
void c7x_l1d_enable_wt(void);

#endif /* ZEPHYR_INCLUDE_ARCH_C7X_CACHE_H_ */
