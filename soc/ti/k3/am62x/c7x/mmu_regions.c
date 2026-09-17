/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 *
 * The identity regions of the SDK gMmuRegionConfig[]. Without them CLEC is dead.
 */

#include <zephyr/arch/c7x/mmu.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#define BUS_RANGE_ENTRY(node, idx)						\
	C7X_MMU_REGION_ENTRY(DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(node, idx),	\
			     DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(node, idx),	\
			     DT_RANGES_LENGTH_BY_IDX(node, idx), C7X_MT_DEVICE),

static const struct c7x_mmu_region mmu_regions[] = {
	DT_FOREACH_RANGE(DT_NODELABEL(cbass_main), BUS_RANGE_ENTRY)

	C7X_MMU_REGION_DT_FLAT_ENTRY(DT_NODELABEL(clec), C7X_MT_DEVICE)

	C7X_MMU_REGION_FLAT_ENTRY(DT_REG_ADDR_BY_NAME(DT_NODELABEL(c7x_dru), dru),
				  ROUND_UP(DT_REG_SIZE_BY_NAME(DT_NODELABEL(c7x_dru), dru),
					   MB(2)), C7X_MT_DEVICE),

	C7X_MMU_REGION_FLAT_ENTRY(DT_REG_ADDR(DT_NODELABEL(l2sram)),
				  ROUND_UP(DT_REG_SIZE(DT_NODELABEL(l2sram)), MB(2)),
				  C7X_MT_NORMAL),

	C7X_MMU_REGION_DT_FLAT_ENTRY(DT_NODELABEL(l2aux), C7X_MT_NORMAL)

	C7X_MMU_REGION_FLAT_ENTRY(DT_REG_ADDR(DT_NODELABEL(ddr_ipc)),
				  DT_REG_ADDR(DT_NODELABEL(stacks)) +
				  DT_REG_SIZE(DT_NODELABEL(stacks)) -
				  DT_REG_ADDR(DT_NODELABEL(ddr_ipc)), C7X_MT_NORMAL_NC),

	C7X_MMU_REGION_FLAT_ENTRY(DT_REG_ADDR(DT_NODELABEL(vectors)),
				  DT_REG_ADDR(DT_NODELABEL(stacks)) +
				  DT_REG_SIZE(DT_NODELABEL(stacks)) -
				  DT_REG_ADDR(DT_NODELABEL(vectors)), C7X_MT_NORMAL),

	/* after the ddr_ipc entry, which these may lie inside */
	C7X_MMU_REGION_DT_COMPAT_FOREACH_FLAT_ENTRY_FROM_DT(ti_c7x_mmu_region)

	C7X_MMU_REGION_DT_FLAT_ENTRY(DT_NODELABEL(ddr0_reserved), C7X_MT_NORMAL_NC)
};

const struct c7x_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
