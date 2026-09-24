/* Linux remoteproc parses this table at fixed addresses. These are not diagnostic
 * markers, so do not relocate them. */
/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 *
 *    0x99900000  .resource_table (this table, 0x8C = 140 bytes)
 */

#include "rsc_table.h"
#include <stddef.h>
#include <zephyr/kernel.h>

BUILD_ASSERT(sizeof(struct am62d_resource_table) == RSC_TABLE_EXPECTED_SIZE,
	     "am62d_resource_table size mismatch: expected 140 bytes");

BUILD_ASSERT(offsetof(struct am62d_resource_table, vdev) == 0x18U, "vdev offset must be 0x18");
BUILD_ASSERT(offsetof(struct am62d_resource_table, trace) == 0x5CU, "trace offset must be 0x5C");

__attribute__((section(".resource_table")))
__attribute__((used)) volatile struct am62d_resource_table am62d_rsc_table = {
	.hdr =
		{
			.ver = 1U,
			.num = 2U,
		},
	.offset =
		{
			offsetof(struct am62d_resource_table, vdev),
			offsetof(struct am62d_resource_table, trace),
		},
	VDEV_ENTRY.trace =
		{
			.type = RSC_TRACE,
			.da = RSC_TRACE_DA,
			.len = RSC_TRACE_LEN,
			.name = RSC_TRACE_NAME,
		},
};
