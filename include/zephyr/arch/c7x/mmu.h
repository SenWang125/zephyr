/*
 *  Copyright (c) 2026 Texas Instruments Incorporated
 *  SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief C7x MMU translation table interface
 */
#ifndef ZEPHYR_INCLUDE_ARCH_C7X_MMU_H_
#define ZEPHYR_INCLUDE_ARCH_C7X_MMU_H_

#include <stdint.h>
#include <stddef.h>

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr.h>
#include <zephyr/sys/util_macro.h>

/* Translation table descriptors */
#define C7X_MMU_DESC_TYPE_MASK	((uint64_t)0x3)
#define C7X_MMU_DESC_INVALID	((uint64_t)0x0)
#define C7X_MMU_DESC_BLOCK	((uint64_t)0x1)
#define C7X_MMU_DESC_TABLE	((uint64_t)0x3)
#define C7X_MMU_DESC_PAGE	((uint64_t)0x3)
#define C7X_MMU_ATTR_IDX(n)	((uint64_t)(n) << 2)
#define C7X_MMU_NS		((uint64_t)1 << 5)
#define C7X_MMU_AP_PRW		((uint64_t)0 << 6)
#define C7X_MMU_SH_OUTER	((uint64_t)2 << 8)
#define C7X_MMU_AF		((uint64_t)1 << 10)
#define C7X_MMU_UXN		((uint64_t)1 << 54)

#define C7X_MMU_ENTRIES		512U
#define C7X_MMU_INDEX_MASK	(C7X_MMU_ENTRIES - 1U)
#define C7X_MMU_LEVEL_SHIFT(l)	(39U - 9U * (l))
#define C7X_MMU_PAGE_SHIFT	12U
#define C7X_MMU_PAGE_SIZE	0x1000U
#define C7X_MMU_PAGE_MASK	((uint64_t)C7X_MMU_PAGE_SIZE - 1U)
#define C7X_MMU_BLOCK_SIZE	((uint64_t)1 << 21)
#define C7X_MMU_BLOCK_MASK	(C7X_MMU_BLOCK_SIZE - 1U)
#define C7X_MMU_UPPER_ATTRS	(~(uint64_t)0xFFFFFFFFFFFFULL)

/* Tables in the page-table pool, each entry is stored as two 32-bit words */
#define C7X_MMU_POOL_TABLES	16U
#define C7X_MMU_POOL_WORDS	(C7X_MMU_POOL_TABLES * C7X_MMU_ENTRIES * 2U)

/* MAIR byte for each attribute index*/
#define C7X_MAIR0		0x00U
#define C7X_MAIR1		0x04U
#define C7X_MAIR2		0x08U
#define C7X_MAIR3		0x0cU
#define C7X_MAIR4		0x44U
#define C7X_MAIR5		0x4fU
#define C7X_MAIR6		0xbbU
#define C7X_MAIR7		0x7dU

/* The attribute indexes this port maps with */
#define C7X_MT_DEVICE		0U	/* MAIR0, device nGnRnE */
#define C7X_MT_DEVICE_nGnRE	1U	/* MAIR1, device nGnRE */
#define C7X_MT_DEVICE_nGRE	2U	/* MAIR2, device nGRE */
#define C7X_MT_DEVICE_GRE	3U	/* MAIR3, device GRE */
#define C7X_MT_NORMAL_NC	4U	/* MAIR4, normal non-cacheable */
#define C7X_MT_NORMAL_INNER_WB	5U	/* MAIR5, normal inner write-back, outer non-cacheable */
#define C7X_MT_NORMAL_WT	6U	/* MAIR6, normal write-through */
#define C7X_MT_NORMAL		7U	/* MAIR7, normal write-back */
#define C7X_MT_MASK		0x7U

/** The TLB_INV operand that invalidates every entry */
#define C7X_TLB_INV_ALL		0U

#define C7X_TCR_WALK_EN		((uint64_t)1)
#define C7X_TCR_ADDR_BITS(n)	((uint64_t)(64U - (n)) << 1)

struct c7x_mmu_region {
	uintptr_t base_pa;
	uintptr_t base_va;
	size_t size;
	uint32_t attrs;
};

struct c7x_mmu_config {
	unsigned int num_regions;
	const struct c7x_mmu_region *mmu_regions;
};

#define C7X_MMU_REGION_ENTRY(_base_pa, _base_va, _size, _attrs) \
	{							\
		.base_pa = (_base_pa),				\
		.base_va = (_base_va),				\
		.size = (_size),				\
		.attrs = (_attrs),				\
	}

#define C7X_MMU_REGION_FLAT_ENTRY(adr, sz, attrs) C7X_MMU_REGION_ENTRY(adr, adr, sz, attrs)

/**
 * @brief MMU region entry for a devicetree node's first reg bank.
 *
 * @param node_id Devicetree node identifier.
 * @param attrs C7X_MT_* attribute index.
 */
#define C7X_MMU_REGION_DT_FLAT_ENTRY(node_id, attrs) \
	C7X_MMU_REGION_FLAT_ENTRY(DT_REG_ADDR(node_id), DT_REG_SIZE(node_id), attrs),

/**
 * @brief Convert a DT zephyr,memory-attr value to a C7X_MT_* attribute index.
 *
 * @param dt_attr The DT memory attribute value from zephyr,memory-attr.
 */
#define C7X_DT_MEM_ATTR_TO_MT(dt_attr) \
	((DT_MEM_ATTR_GET(dt_attr) & DT_MEM_CACHEABLE) ? C7X_MT_NORMAL : C7X_MT_NORMAL_NC)

/**
 * @brief MMU region entry whose attribute comes from the node's zephyr,memory-attr.
 *
 * Nodes without the property are skipped.
 *
 * @param node_id Devicetree node identifier.
 */
#define C7X_MMU_REGION_DT_FLAT_ENTRY_FROM_DT(node_id)			\
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, zephyr_memory_attr),	\
		(C7X_MMU_REGION_DT_FLAT_ENTRY(node_id,			\
			C7X_DT_MEM_ATTR_TO_MT(				\
				DT_PROP(node_id, zephyr_memory_attr)))))

/**
 * @brief MMU region entries for every status okay node of @p compat.
 *
 * @param compat Devicetree compatible to iterate over.
 */
#define C7X_MMU_REGION_DT_COMPAT_FOREACH_FLAT_ENTRY_FROM_DT(compat) \
	DT_FOREACH_STATUS_OKAY(compat, C7X_MMU_REGION_DT_FLAT_ENTRY_FROM_DT)

extern const struct c7x_mmu_config mmu_config;

extern uint32_t *c7x_mmu_l0_root;

uint32_t *mmu_get_tables_base(void);

void c7x_mmu_mair_set(unsigned int index, unsigned int value);
void c7x_mmu_tcr_set(uint64_t tcr);
void c7x_mmu_tbr0_set(uint32_t *table);
void c7x_mmu_tlb_inv(uint64_t val);
void c7x_mmu_enable(void);

void c7x_mm_init(void);

#endif /* ZEPHYR_INCLUDE_ARCH_C7X_MMU_H_ */
