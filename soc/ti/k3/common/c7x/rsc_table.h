/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_IPC_RSC_TABLE_H_
#define SRC_IPC_RSC_TABLE_H_

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <resource_table.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RSC_TRACE_DA  DT_REG_ADDR(DT_NODELABEL(trace_buf))
#define RSC_TRACE_LEN DT_REG_SIZE(DT_NODELABEL(trace_buf))

#define RSC_TRACE_NAME "trace:c7x_0"

/* struct fw_resource_table with both entries, which Zephyr only builds with CONFIG_RAM_CONSOLE */
METAL_PACKED_BEGIN
struct am62d_resource_table {
	struct resource_table hdr;
	uint32_t offset[2];

	struct fw_rsc_vdev vdev;
	struct fw_rsc_vdev_vring vring0;
	struct fw_rsc_vdev_vring vring1;

	struct fw_rsc_trace trace;
} METAL_PACKED_END;

#define RSC_TABLE_EXPECTED_SIZE 140U

extern volatile struct am62d_resource_table am62d_rsc_table;

#ifdef __cplusplus
}
#endif

#endif /* SRC_IPC_RSC_TABLE_H_ */
