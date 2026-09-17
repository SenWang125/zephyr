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

/* struct arch_esf offsets, used by the fault entry in exception.S through
 * .cdecls and pinned to the structure by BUILD_ASSERT in fatal.c
 */
#define C7X_ESF_RP		0x00
#define C7X_ESF_NRP		0x08
#define C7X_ESF_NTSR		0x10
#define C7X_ESF_TSR		0x18
#define C7X_ESF_IERR		0x20
#define C7X_ESF_IESR		0x28
#define C7X_ESF_IEAR		0x30
#define C7X_ESF_ECSP		0x38
#define C7X_ESF_SP		0x40
#define C7X_ESF_VECTOR		0x48
#define C7X_ESF_A0		0x50
#define C7X_EXC_FRAME_SIZE	0xd0

/* Which vector the fault came from. */
#define C7X_FAULT_VECTOR_EXC	0
#define C7X_FAULT_VECTOR_PF	1

#define C7X_ISR_SCALAR_SIZE		896
#define C7X_ISR_VECTOR_SIZE		3968

/* aligning the vector base to 64 from a 16-aligned SP discards up to 48 bytes */
#define C7X_ISR_ALIGN_DISCARD		48
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
