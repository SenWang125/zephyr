/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 *
 * Page table pool in the image's own .data (DDR), 4 KB aligned, like the MCU+ SDK Mmu_tableArray.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <zephyr/cache.h>
#include <zephyr/arch/c7x/cache.h>
#include <zephyr/arch/c7x/mmu.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

BUILD_ASSERT(C7X_MMU_LEVEL_SHIFT(C7X_MMU_LAST_LEVEL) == C7X_MMU_PAGE_SHIFT,
	     "C7x MMU level count does not match the page shift");

#define C7X_MMU_POOL_BYTES	(C7X_MMU_POOL_WORDS * sizeof(uint32_t))

#define BLK_BASE       (C7X_MMU_DESC_BLOCK | C7X_MMU_NS | C7X_MMU_AP_PRW | C7X_MMU_SH_OUTER | \
			C7X_MMU_AF | C7X_MMU_UXN)

#define PAGE_OF_BLK(b) (((b) & ~C7X_MMU_DESC_TYPE_MASK) | C7X_MMU_DESC_PAGE)

#define PAGE_BASE (C7X_MMU_DESC_PAGE | C7X_MMU_NS | C7X_MMU_AP_PRW | C7X_MMU_SH_OUTER | \
		   C7X_MMU_AF | C7X_MMU_UXN)

#pragma DATA_SECTION(c7x_mmu_tables, ".data:c7x_mmu_tables")
#pragma DATA_ALIGN(c7x_mmu_tables, 4096)
static uint32_t c7x_mmu_tables[C7X_MMU_POOL_WORDS];
static uint32_t c7x_mmu_next_slot;
static uint32_t *c7x_mmu_l0_root = c7x_mmu_tables;

uint32_t *c7x_mmu_get_tables_base(void)
{
	return *(uint32_t *volatile *)&c7x_mmu_l0_root;
}

static uint32_t c7x_mmu_get_next_slot(void)
{
	return *(volatile uint32_t *)&c7x_mmu_next_slot;
}

static void c7x_mmu_set_next_slot(uint32_t slot)
{
	*(volatile uint32_t *)&c7x_mmu_next_slot = slot;
}

static __noinline
uint32_t *c7x_mmu_alloc_table(void)
{
	uint32_t slot = c7x_mmu_get_next_slot();
	uint32_t *base;
	uint32_t i;

	if (slot >= C7X_MMU_POOL_TABLES) {
		return 0;
	}

	base = c7x_mmu_get_tables_base();
	c7x_mmu_set_next_slot(slot + 1U);

	base += (uint32_t)(slot * C7X_MMU_ENTRIES * 2U);
	for (i = 0U; i < C7X_MMU_ENTRIES * 2U; i++) {
		base[i] = 0U;
	}
	return base;
}

static void c7x_mmu_write_entry(uint32_t *table, uint32_t idx, uint64_t desc)
{
	table[idx * 2U]     = (uint32_t)(desc & 0xFFFFFFFFU);
	table[idx * 2U + 1] = (uint32_t)(desc >> 32);
}

static uint64_t c7x_mmu_read_entry(const uint32_t *table, uint32_t idx)
{
	uint64_t hi = table[idx * 2U + 1];
	uint64_t lo = table[idx * 2U];

	return (hi << 32) | lo;
}

static volatile uint32_t c7x_mmu_fail[2] __used __aligned(64) Z_GENERIC_SECTION(.bss:c7x_diag);

__noinline
static void mmu_fail(uint32_t code, uint64_t va)
{
	c7x_mmu_fail[0] = 0xBAD00000U | (code & 0xFFFFU);
	c7x_mmu_fail[1] = (uint32_t)va;
	for (;;) {
	}
}

static __noinline void mmu_map_one(uint32_t *l0, uint64_t va, uint64_t pa,
						  uint64_t attr)
{
	uint32_t i0 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(0)) & C7X_MMU_INDEX_MASK);
	uint32_t i1 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(1)) & C7X_MMU_INDEX_MASK);
	uint32_t i2 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(2)) & C7X_MMU_INDEX_MASK);
	uint32_t *l1, *l2;
	uint64_t e;

	e = c7x_mmu_read_entry(l0, i0);
	if ((e & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_TABLE) {
		l1 = (uint32_t *)(uintptr_t)(e & ~C7X_MMU_PAGE_MASK);
	} else {
		l1 = c7x_mmu_alloc_table();
		if (!l1) {
			mmu_fail(5U, va);
		}
		c7x_mmu_write_entry(l0, i0, (uint64_t)(uintptr_t)l1 | C7X_MMU_DESC_TABLE);
	}

	e = c7x_mmu_read_entry(l1, i1);
	if ((e & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_TABLE) {
		l2 = (uint32_t *)(uintptr_t)(e & ~C7X_MMU_PAGE_MASK);
	} else {
		l2 = c7x_mmu_alloc_table();
		if (!l2) {
			mmu_fail(6U, va);
		}
		c7x_mmu_write_entry(l1, i1, (uint64_t)(uintptr_t)l2 | C7X_MMU_DESC_TABLE);
	}

	c7x_mmu_write_entry(l2, i2, (pa & ~C7X_MMU_BLOCK_MASK) | attr);
}

__noinline
static uint32_t *mmu_walk_l2(uint32_t *l0, uint64_t va)
{
	uint32_t i0 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(0)) & C7X_MMU_INDEX_MASK);
	uint32_t i1 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(1)) & C7X_MMU_INDEX_MASK);
	uint32_t *l1, *l2;
	uint64_t e;

	e = c7x_mmu_read_entry(l0, i0);
	if ((e & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_TABLE) {
		l1 = (uint32_t *)(uintptr_t)(e & ~C7X_MMU_PAGE_MASK);
	} else {
		l1 = c7x_mmu_alloc_table();
		if (!l1) {
			return 0;
		}
		c7x_mmu_write_entry(l0, i0, (uint64_t)(uintptr_t)l1 | C7X_MMU_DESC_TABLE);
	}

	e = c7x_mmu_read_entry(l1, i1);
	if ((e & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_TABLE) {
		l2 = (uint32_t *)(uintptr_t)(e & ~C7X_MMU_PAGE_MASK);
	} else {
		l2 = c7x_mmu_alloc_table();
		if (!l2) {
			return 0;
		}
		c7x_mmu_write_entry(l1, i1, (uint64_t)(uintptr_t)l2 | C7X_MMU_DESC_TABLE);
	}
	return l2;
}

__noinline
static void mmu_map_page(uint32_t *l0, uint64_t va, uint64_t pa, uint64_t attr)
{
	uint32_t i2 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(2)) & C7X_MMU_INDEX_MASK);
	uint32_t i3 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(3)) & C7X_MMU_INDEX_MASK);
	uint32_t *l2, *l3;
	uint64_t e;
	uint32_t k;

	l2 = mmu_walk_l2(l0, va);
	if (!l2) {
		mmu_fail(2U, va);
	}

	e = c7x_mmu_read_entry(l2, i2);
	if ((e & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_TABLE) {
		l3 = (uint32_t *)(uintptr_t)(e & ~C7X_MMU_PAGE_MASK);
	} else {
		uint64_t old_attr = e & C7X_MMU_PAGE_MASK;
		uint64_t old_pa   = e & ~C7X_MMU_PAGE_MASK;
		uint64_t old_hi   = e & C7X_MMU_UPPER_ATTRS;

		l3 = c7x_mmu_alloc_table();
		if (!l3) {
			mmu_fail(3U, va);
		}
		if ((e & C7X_MMU_DESC_TYPE_MASK) != 0ULL) {
			old_pa &= ~C7X_MMU_BLOCK_MASK;
			for (k = 0U; k < C7X_MMU_ENTRIES; k++) {
				c7x_mmu_write_entry(l3, k,
					(old_pa + ((uint64_t)k << C7X_MMU_PAGE_SHIFT)) |
					PAGE_OF_BLK(old_attr) | old_hi);
			}
		}
		c7x_mmu_write_entry(l2, i2, (uint64_t)(uintptr_t)l3 | C7X_MMU_DESC_TABLE);
	}

	c7x_mmu_write_entry(l3, i3, (pa & ~C7X_MMU_PAGE_MASK) | PAGE_OF_BLK(attr));
}

__noinline
static void c7x_mmu_map(uint32_t *l0, uint64_t va, uint64_t pa, uint64_t size, uint32_t attr_idx)
{
	/* attr_idx comes from 3-bit config fields and c7x_mm_init programs all
	 * eight MAIR bytes; the index selects one, it cannot be out of range.
	 */
	uint64_t attr = BLK_BASE | C7X_MMU_ATTR_IDX(attr_idx & C7X_MT_MASK);

	if (((va | pa | size) & C7X_MMU_PAGE_MASK) != 0ULL) {
		mmu_fail(4U, va);
	}

	while (size != 0ULL) {
		if ((size >= C7X_MMU_BLOCK_SIZE) &&
		    (((va | pa) & C7X_MMU_BLOCK_MASK) == 0ULL)) {
			mmu_map_one(l0, va, pa, attr);
			va   += C7X_MMU_BLOCK_SIZE;
			pa   += C7X_MMU_BLOCK_SIZE;
			size -= C7X_MMU_BLOCK_SIZE;
		} else {
			mmu_map_page(l0, va, pa, attr);
			va   += (uint64_t)C7X_MMU_PAGE_SIZE;
			pa   += (uint64_t)C7X_MMU_PAGE_SIZE;
			size -= (uint64_t)C7X_MMU_PAGE_SIZE;
		}
	}
}

__noinline void c7x_mmu_enable(void)
{
	__asm__ volatile (" MVK64 .L1 0x80000000000000C1, A2\n"
			  " MVC .S1 SCR, A3\n"
			  " NOP 5\n"
			  " ORD .L1 A2, A3, A3\n"
			  " MVC .S1 A3, SCR\n"
			  " NOP 6\n");
}

void c7x_mm_init(void)
{
	const struct c7x_mmu_region *r;
	uint32_t *l0;
	unsigned int i;

	c7x_mmu_mair_set(0U, C7X_MAIR0);
	c7x_mmu_mair_set(1U, C7X_MAIR1);
	c7x_mmu_mair_set(2U, C7X_MAIR2);
	c7x_mmu_mair_set(3U, C7X_MAIR3);
	c7x_mmu_mair_set(4U, C7X_MAIR4);
	c7x_mmu_mair_set(5U, C7X_MAIR5);
	c7x_mmu_mair_set(6U, C7X_MAIR6);
	c7x_mmu_mair_set(7U, C7X_MAIR7);

	c7x_mmu_tcr_set(C7X_TCR_ADDR_BITS(48U) | C7X_TCR_WALK_EN);

	c7x_mmu_set_next_slot(0U);
	l0 = c7x_mmu_alloc_table();
	for (i = 0U; i < mmu_config.num_regions; i++) {
		r = &mmu_config.mmu_regions[i];
		c7x_mmu_map(l0, (uint64_t)r->base_va, (uint64_t)r->base_pa, (uint64_t)r->size, r->attrs);
	}

	c7x_l1d_wbinv(C7X_L1D_WBINV_ALL);

	c7x_mmu_tbr0_set(l0);

	c7x_mmu_enable();

	/* L1D cache startup, after the MMU is on: write-through, then invalidate. */
	c7x_l1d_enable_wt();

	c7x_mmu_tlb_inv(C7X_TLB_INV_ALL);
}

/* Every page is privileged read-write and user-execute-never, so a user-mode
 * mapping is the one request this port cannot satisfy.
 */
#define C7X_MMU_FLAGS_UNSUPPORTED	K_MEM_PERM_USER

static int c7x_mmu_attr_idx(uint32_t flags, uint32_t *attr_idx)
{
	if ((flags & C7X_MMU_FLAGS_UNSUPPORTED) != 0U) {
		return -ENOTSUP;
	}

	switch (flags & K_MEM_CACHE_MASK) {
	case K_MEM_CACHE_WB:
		*attr_idx = C7X_MT_NORMAL;
		break;
	case K_MEM_CACHE_WT:
		*attr_idx = C7X_MT_NORMAL_WT;
		break;
	case K_MEM_C7X_DEVICE_nGnRnE:
		*attr_idx = C7X_MT_DEVICE;
		break;
	case K_MEM_C7X_DEVICE_nGnRE:
		*attr_idx = C7X_MT_DEVICE_nGnRE;
		break;
	case K_MEM_C7X_DEVICE_nGRE:
		*attr_idx = C7X_MT_DEVICE_nGRE;
		break;
	case K_MEM_C7X_DEVICE_GRE:
		*attr_idx = C7X_MT_DEVICE_GRE;
		break;
	case K_MEM_C7X_NORMAL_NC:
		*attr_idx = C7X_MT_NORMAL_NC;
		break;
	case K_MEM_C7X_NORMAL_INNER_WB:
		*attr_idx = C7X_MT_NORMAL_INNER_WB;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static uint32_t *c7x_mmu_next_table(uint32_t *table, uint32_t idx)
{
	uint64_t e = c7x_mmu_read_entry(table, idx);
	uint32_t *child;

	if ((e & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_TABLE) {
		return (uint32_t *)(uintptr_t)(e & ~C7X_MMU_PAGE_MASK);
	}
	if ((e & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_BLOCK) {
		return (uint32_t *)0;
	}
	child = c7x_mmu_alloc_table();
	if (child == (uint32_t *)0) {
		return (uint32_t *)0;
	}
	c7x_mmu_write_entry(table, idx, (uint64_t)(uintptr_t)child | C7X_MMU_DESC_TABLE);
	return child;
}

static int c7x_mmu_map_page(uint32_t *l0, uint64_t va, uint64_t pa, uint32_t attr_idx)
{
	uint32_t i0 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(0)) & C7X_MMU_INDEX_MASK);
	uint32_t i1 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(1)) & C7X_MMU_INDEX_MASK);
	uint32_t i2 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(2)) & C7X_MMU_INDEX_MASK);
	uint32_t i3 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(3)) & C7X_MMU_INDEX_MASK);
	uint32_t *l1, *l2, *l3;

	l1 = c7x_mmu_next_table(l0, i0);
	if (l1 == (uint32_t *)0) {
		return -ENOMEM;
	}
	l2 = c7x_mmu_next_table(l1, i1);
	if (l2 == (uint32_t *)0) {
		return -ENOMEM;
	}
	l3 = c7x_mmu_next_table(l2, i2);
	if (l3 == (uint32_t *)0) {
		return ((c7x_mmu_read_entry(l2, i2) & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_BLOCK)
			       ? -EINVAL
			       : -ENOMEM;
	}
	if ((c7x_mmu_read_entry(l3, i3) & C7X_MMU_DESC_TYPE_MASK) != C7X_MMU_DESC_INVALID) {
		return -EBUSY;
	}
	c7x_mmu_write_entry(l3, i3, (pa & ~C7X_MMU_PAGE_MASK) | PAGE_BASE | C7X_MMU_ATTR_IDX(attr_idx));
	return 0;
}

static void c7x_mmu_unmap_page(uint32_t *l0, uint64_t va)
{
	uint32_t i0 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(0)) & C7X_MMU_INDEX_MASK);
	uint32_t i1 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(1)) & C7X_MMU_INDEX_MASK);
	uint32_t i2 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(2)) & C7X_MMU_INDEX_MASK);
	uint32_t i3 = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(3)) & C7X_MMU_INDEX_MASK);
	uint64_t e;
	uint32_t *l1, *l2, *l3;

	e = c7x_mmu_read_entry(l0, i0);
	if ((e & C7X_MMU_DESC_TYPE_MASK) != C7X_MMU_DESC_TABLE) {
		return;
	}
	l1 = (uint32_t *)(uintptr_t)(e & ~C7X_MMU_PAGE_MASK);
	e = c7x_mmu_read_entry(l1, i1);
	if ((e & C7X_MMU_DESC_TYPE_MASK) != C7X_MMU_DESC_TABLE) {
		return;
	}
	l2 = (uint32_t *)(uintptr_t)(e & ~C7X_MMU_PAGE_MASK);
	e = c7x_mmu_read_entry(l2, i2);
	if ((e & C7X_MMU_DESC_TYPE_MASK) != C7X_MMU_DESC_TABLE) {
		return;
	}
	l3 = (uint32_t *)(uintptr_t)(e & ~C7X_MMU_PAGE_MASK);
	c7x_mmu_write_entry(l3, i3, C7X_MMU_DESC_INVALID);
}

static int c7x_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	uintptr_t va = (uintptr_t)virt;
	uint32_t attr_idx;
	uint32_t *l0;
	size_t off;
	unsigned int key;
	int rc;

	rc = c7x_mmu_attr_idx(flags, &attr_idx);
	if (rc != 0) {
		return rc;
	}
	if (((va | phys | (uintptr_t)size) & C7X_MMU_PAGE_MASK) != 0U) {
		return -EINVAL;
	}
	if (size == 0U) {
		return 0;
	}

	key = irq_lock();
	l0 = c7x_mmu_get_tables_base();
	for (off = 0U; off < size; off += C7X_MMU_PAGE_SIZE) {
		rc = c7x_mmu_map_page(l0, (uint64_t)(va + off),
				      (uint64_t)(phys + off), attr_idx);
		if (rc != 0) {
			break;
		}
	}
	(void)arch_dcache_flush_range((void *)c7x_mmu_get_tables_base(), C7X_MMU_POOL_BYTES);
	if (rc == 0) {
		c7x_mmu_tlb_inv(C7X_TLB_INV_ALL);
	}
	irq_unlock(key);
	return rc;
}

static int c7x_mem_unmap(void *addr, size_t size)
{
	uintptr_t va = (uintptr_t)addr;
	uint32_t *l0;
	size_t off;
	unsigned int key;

	if (((va | (uintptr_t)size) & C7X_MMU_PAGE_MASK) != 0U) {
		return -EINVAL;
	}
	if (size == 0U) {
		return 0;
	}

	key = irq_lock();
	l0 = c7x_mmu_get_tables_base();
	for (off = 0U; off < size; off += C7X_MMU_PAGE_SIZE) {
		c7x_mmu_unmap_page(l0, (uint64_t)(va + off));
	}
	(void)arch_dcache_flush_range((void *)c7x_mmu_get_tables_base(), C7X_MMU_POOL_BYTES);
	c7x_mmu_tlb_inv(C7X_TLB_INV_ALL);
	irq_unlock(key);
	return 0;
}

void arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	int rc = c7x_mem_map(virt, phys, size, flags);

	if (rc != 0) {
		LOG_ERR("map %p <- 0x%lx size 0x%zx: %d", virt, (unsigned long)phys, size, rc);
		k_panic();
	}
}

void arch_mem_unmap(void *addr, size_t size)
{
	(void)c7x_mem_unmap(addr, size);
}

int arch_page_phys_get(void *virt, uintptr_t *phys)
{
	uint64_t va = (uint64_t)(uintptr_t)virt;
	uint32_t *table = c7x_mmu_get_tables_base();
	uint64_t e = 0U;
	unsigned int level;

	for (level = 0U; level < C7X_MMU_LEVELS; level++) {
		uint32_t idx = (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(level)) & C7X_MMU_INDEX_MASK);
		uint64_t out_mask = ((uint64_t)1 << C7X_MMU_LEVEL_SHIFT(level)) - 1U;

		e = c7x_mmu_read_entry(table, idx);
		if ((e & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_INVALID ||
		    (level == C7X_MMU_LAST_LEVEL &&
		     (e & C7X_MMU_DESC_TYPE_MASK) != C7X_MMU_DESC_PAGE)) {
			return -EFAULT;
		}
		if (level == C7X_MMU_LAST_LEVEL ||
		    (e & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_BLOCK) {
			if (phys != NULL) {
				*phys = (uintptr_t)(((e & ~C7X_MMU_UPPER_ATTRS) & ~out_mask) |
						    (va & out_mask));
			}
			return 0;
		}
		table = (uint32_t *)(uintptr_t)(e & ~C7X_MMU_UPPER_ATTRS & ~C7X_MMU_PAGE_MASK);
	}
	return -EFAULT;
}
