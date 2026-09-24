/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief C7x inline kernel interface functions
 */

#ifndef ZEPHYR_INCLUDE_ARCH_C7X_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ARCH_C7X_ARCH_INLINES_H_

#include <zephyr/kernel_structs.h>

static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
	return 1U;
}

#endif /* ZEPHYR_INCLUDE_ARCH_C7X_ARCH_INLINES_H_ */
