/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_C7X_INCLUDE_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_C7X_INCLUDE_KERNEL_ARCH_DATA_H_

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

#ifndef _ASMLANGUAGE
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The switch saves thread's context in a frame from its own stack per EABI
 *
 *    Callee-set frame: A8-A15, B14, B15, RP, TCSP, TSR.
 *    +0      [0]    reserved (16-byte ABI free area, lower word)
 *    +8      [1]    reserved (16-byte ABI free area, upper word)
 *    +16     [2]    A8  (callee-saved)
 *    +24     [3]    A9  (callee-saved)
 *    +32     [4]    A10 (callee-saved)
 *    +40     [5]    A11 (callee-saved)
 *    +48     [6]    A12 (callee-saved)
 *    +56     [7]    A13 (callee-saved)
 *    +64     [8]    A14 (callee-saved)
 *    +72     [9]    A15 (callee-saved)
 *    +80     [10]   B14 (callee-saved)
 *    +88     [11]   B15 (callee-saved)
 *    +96     [12]   TCSP (thread context save pointer)
 *    +104    [13]   TSR  (task state register)
 *    +112    [14]   glue_rp (entry trampoline address)
 *    +120    [15]   RP   (resume PC and same as glue_rp for new threads)
 *
 *  Total: 16 slots * 8 bytes = 128 bytes.
 *  B10-B13 are caller-save instead.
 */
struct c7x_switch_frame {
	uint64_t reserved[2];
	uint64_t a8;
	uint64_t a9;
	uint64_t a10;
	uint64_t a11;
	uint64_t a12;
	uint64_t a13;
	uint64_t a14;
	uint64_t a15;
	uint64_t b14;
	uint64_t b15;
	uint64_t tcsp;
	uint64_t tsr;
	uint64_t glue_rp;
	uint64_t rp;
};

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_C7X_INCLUDE_KERNEL_ARCH_DATA_H_ */
