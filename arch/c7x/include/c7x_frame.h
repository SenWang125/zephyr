/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_C7X_INCLUDE_C7X_FRAME_H_
#define ZEPHYR_ARCH_C7X_INCLUDE_C7X_FRAME_H_

/* Context frame with one 64-bit slot per entry below the saved stack pointer */
#define C7X_CFRAME_SLOTS		16
#define C7X_CFRAME_SLOT_RESERVED0	0
#define C7X_CFRAME_SLOT_RESERVED1	1
#define C7X_CFRAME_SLOT_A8		2
#define C7X_CFRAME_SLOT_A9		3
#define C7X_CFRAME_SLOT_A10		4
#define C7X_CFRAME_SLOT_A11		5
#define C7X_CFRAME_SLOT_A12		6
#define C7X_CFRAME_SLOT_A13		7
#define C7X_CFRAME_SLOT_A14		8
#define C7X_CFRAME_SLOT_A15		9
#define C7X_CFRAME_SLOT_B14		10
#define C7X_CFRAME_SLOT_B15		11
#define C7X_CFRAME_SLOT_TCSP		12
#define C7X_CFRAME_SLOT_TSR		13
#define C7X_CFRAME_SLOT_GLUE_RP		14
#define C7X_CFRAME_SLOT_RP		15

#endif /* ZEPHYR_ARCH_C7X_INCLUDE_C7X_FRAME_H_ */
