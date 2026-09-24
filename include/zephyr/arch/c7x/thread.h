/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief C7x per-thread architecture data
 */

#ifndef ZEPHYR_INCLUDE_ARCH_C7X_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_C7X_THREAD_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The switch saves a thread's context in a frame on its own stack (switch_handle). */
struct _callee_saved {
	char dummy;
};
typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	char dummy;
};
typedef struct _thread_arch _thread_arch_t;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_C7X_THREAD_H_ */
