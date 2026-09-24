/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __K3_CTRL_PARTITIONS_H_
#define __K3_CTRL_PARTITIONS_H_

#define K3_CTRL_MMR_KICK0_UNLOCK_VAL (0x68EF3490U)
#define K3_CTRL_MMR_KICK1_UNLOCK_VAL (0xD172BC5AU)
#define K3_CTRL_MMR_KICK_LOCK_VAL   (0x00000000U)

void k3_unlock_all_ctrl_partitions(void);

#endif /* __K3_CTRL_PARTITIONS_H */
