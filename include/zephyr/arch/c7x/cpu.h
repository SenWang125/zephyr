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

#define C7X_CONTEXT_SAVE_SIZE KB(8)

/* TSR.COP Default Task Mode, that is above all event priority (0x00-0xE0) */
#define C7X_TSR_COP_TASK_MODE 0xFFU

/* TSR.COP POR and fault lockout, all events disabled */
#define C7X_TSR_COP_LOCKOUT 0x1FFU

/* TSR.GEE: events are globally enabled */
#define C7X_TSR_GEE BIT64(25)

/* TSR: an MMA is attached to this core */
#define C7X_TSR_HWA BIT64(52)

/* DNUM: the cluster number of this core */
#define C7X_DNUM_CLUSTER_MASK GENMASK(23, 16)

/* TSR.CXM execution modes. */
#define C7X_CXM_GU 0U /* Guest user */
#define C7X_CXM_GS 1U /* Guest supervisor */
#define C7X_CXM_U  2U /* Root user */
#define C7X_CXM_S  3U /* Root supervisor */
#define C7X_CXM_SU 4U /* Secure user */
#define C7X_CXM_SS 5U /* Secure supervisor */

#define C7X_TSR_CXM_MASK GENMASK(2, 0)
#define C7X_TSR_COP_MASK GENMASK(16, 8)

/* ISRs are serviced only in supervisor modes. */
#define C7X_CXM_IS_SUPERVISOR(cxm)                                                                 \
	((cxm) == C7X_CXM_GS || (cxm) == C7X_CXM_S || (cxm) == C7X_CXM_SS)

/* EPRI priority field: 1 is the highest priority, 7 the lowest */
#define C7X_EPRI_PRIO_SHIFT 5U
#define C7X_EPRI_PRIO_MIN   1U
#define C7X_EPRI_PRIO_MAX   7U

/* ECSP holds one record per nesting level */
#define C7X_ECSP_NEST_LEVELS 8U
#define C7X_ECSP_SIZE        (C7X_ECSP_NEST_LEVELS * C7X_CONTEXT_SAVE_SIZE)
#define C7X_ECSP_NEST_SHIFT  13U
#define C7X_ECSP_NEST_MASK   GENMASK(15, 13)

#define C7X_L1DCFG_ON BIT64(0)
#define C7X_L1DCFG_WB BIT64(4)

#endif /* ZEPHYR_INCLUDE_ARCH_C7X_CPU_H_ */
