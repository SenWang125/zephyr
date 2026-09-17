/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief C7x instruction set constants
 */

#ifndef ZEPHYR_INCLUDE_ARCH_C7X_CPU_H_
#define ZEPHYR_INCLUDE_ARCH_C7X_CPU_H_

#include <zephyr/sys/util.h>

/* EABI: SP stays 8-byte aligned, with a 16-byte free area at SP */
#define C7X_EABI_SP_ALIGN	8U
#define C7X_EABI_FREE_AREA	16U

/* TSR.COP value that lets every event priority dispatch */
#define C7X_TSR_COP_ALL		0xFFU

/* TSR.GEE: events are globally enabled */
#define C7X_TSR_GEE		BIT64(25)

/* EPRI priority field: 1 is the lowest priority, 7 the highest */
#define C7X_EPRI_PRIO_SHIFT	5U
#define C7X_EPRI_PRIO_MIN	1U
#define C7X_EPRI_PRIO_MAX	7U

#define C7X_CONTEXT_SAVE_SIZE	0x2000U

#define C7X_ECSP_NEST_SHIFT	13U
#define C7X_ECSP_NEST_MASK	GENMASK(15, 13)

/* ECSP holds one record per nesting level */
#define C7X_ECSP_NEST_LEVELS	8U
#define C7X_ECSP_SIZE		(C7X_ECSP_NEST_LEVELS * C7X_CONTEXT_SAVE_SIZE)

#define C7X_L1DCFG_ON		BIT64(0)
#define C7X_L1DCFG_WB		BIT64(4)

#endif /* ZEPHYR_INCLUDE_ARCH_C7X_CPU_H_ */
