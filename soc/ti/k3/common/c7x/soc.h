/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_TI_K3_COMMON_C7X_SOC_H_
#define ZEPHYR_SOC_TI_K3_COMMON_C7X_SOC_H_

#include <zephyr/sys/util_macro.h>

/* Remoteproc control messages carried in the mailbox alongside the vring notify ids */
#define TI_K3_RP_MBOX_SHUTDOWN     0xFFFFFF14U
#define TI_K3_RP_MBOX_SHUTDOWN_ACK 0xFFFFFF15U

/* WKUP_CTRL_MMR clock gate control and its C7x clock gate deactivate bit */
#define TI_K3_WKUP_CLKGATE_CTRL1            0x18284U
#define TI_K3_WKUP_CLKGATE_CTRL1_C7X_NOGATE BIT(11)

#endif /* ZEPHYR_SOC_TI_K3_COMMON_C7X_SOC_H_ */
