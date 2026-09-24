/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief C7x exception frame
 */

#ifndef ZEPHYR_INCLUDE_ARCH_C7X_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_C7X_EXCEPTION_H_

/* Fault vector source (exception or protection-fault) */
#define C7X_FAULT_VECTOR_EXC	0
#define C7X_FAULT_VECTOR_PF	1

#define C7X_ISR_SCALAR_SIZE		896

/* the vector registers (only supports 256bit C7x models) */
#define C7X_VECTOR_BYTES		32

/* the scalar SP slot, then 16 VB, 8 VBL, 8 VBM and 8 CUCR; P0-P7 hold 8 each */
#define C7X_ISR_VEC_SLOTS		41
#define C7X_ISR_SA_SAVE_SIZE		192
#define C7X_ISR_SE_SAVE_SIZE		256

#define C7X_ISR_VECTOR_SIZE		(C7X_ISR_VEC_SLOTS * C7X_VECTOR_BYTES + \
					 4 * C7X_ISR_SA_SAVE_SIZE + \
					 2 * C7X_ISR_SE_SAVE_SIZE + 8 * 8)

/* aligning the vector base to 64 from an 8-aligned SP discards up to 56 bytes */
#define C7X_ISR_ALIGN_DISCARD		56
/* the dispatch CALL takes its outgoing argument area from the same stack */
#define C7X_ISR_OUTGOING_ARGS		32
#define C7X_ISR_FRAME_WORST_CASE	(C7X_ISR_SCALAR_SIZE + C7X_ISR_ALIGN_DISCARD + \
					 C7X_ISR_VECTOR_SIZE + C7X_ISR_OUTGOING_ARGS)

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

struct arch_esf {
	unsigned long long rp;
	unsigned long long nrp;
	unsigned long long ntsr;
	unsigned long long tsr;
	unsigned long long ierr;
	unsigned long long iesr;
	unsigned long long iear;
	unsigned long long ecsp;
	unsigned long long sp;
	unsigned long long vector;
	unsigned long long a[16];
} __packed;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_C7X_EXCEPTION_H_ */
