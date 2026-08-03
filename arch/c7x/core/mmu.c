/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <zephyr/arch/c7x/mmu.h>
#include <kernel_arch_interface.h>
#include <c7x.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

BUILD_ASSERT(C7X_MMU_LEVEL_SHIFT(C7X_MMU_LAST_LEVEL) == C7X_MMU_PAGE_SHIFT,
	     "C7x MMU level count does not match the page shift");
BUILD_ASSERT(CONFIG_KERNEL_VM_SIZE >= CONFIG_SRAM_SIZE * 1024,
	     "the kernel address space is smaller than the RAM");
BUILD_ASSERT(C7X_MMU_POOL_TABLES <= UINT8_MAX + 1U, "table pool slot does not fit a byte");

#define BLK_BASE                                                                                   \
	(C7X_MMU_DESC_BLOCK | C7X_MMU_NS | C7X_MMU_AP_PRW | C7X_MMU_SH_OUTER | C7X_MMU_AF |        \
	 C7X_MMU_UXN)

#define PAGE_OF_BLK(b) (((b) & ~C7X_MMU_DESC_TYPE_MASK) | C7X_MMU_DESC_PAGE)

/* Page table pool in the image's own .noinit, 4 KB aligned. */
#pragma DATA_ALIGN(z_c7x_mmu_tables, 4096)
static uint64_t z_c7x_mmu_tables[C7X_MMU_POOL_ENTRIES] __noinit;
static uint32_t z_c7x_mmu_next_slot __noinit;
static uint64_t *z_c7x_mmu_l0_root = z_c7x_mmu_tables;

/* Valid entries in each pool table */
static uint16_t z_c7x_mmu_used[C7X_MMU_POOL_TABLES] __noinit;

/* Pool tables given back, reused before the ones never allocated */
static uint8_t z_c7x_mmu_free_slot[C7X_MMU_POOL_TABLES] __noinit;
static uint32_t z_c7x_mmu_free_count __noinit;

/* Tables unlinked by one map or unmap, freed after its TLB invalidate */
struct z_c7x_mmu_release {
	uint8_t slot[C7X_MMU_POOL_TABLES];
	uint32_t count;
};

uint64_t *z_c7x_mmu_get_tables_base(void)
{
	return *(uint64_t *volatile *)&z_c7x_mmu_l0_root;
}

static uint32_t z_c7x_mmu_get_next_slot(void)
{
	return *(volatile uint32_t *)&z_c7x_mmu_next_slot;
}

static void z_c7x_mmu_set_next_slot(uint32_t slot)
{
	*(volatile uint32_t *)&z_c7x_mmu_next_slot = slot;
}

static uint32_t z_c7x_mmu_slot(const uint64_t *table)
{
	return (uint32_t)((table - z_c7x_mmu_get_tables_base()) / C7X_MMU_ENTRIES);
}

static uint64_t *z_c7x_mmu_alloc_table(void)
{
	uint32_t slot;
	uint64_t *base;
	uint32_t i;

	if (z_c7x_mmu_free_count != 0U) {
		z_c7x_mmu_free_count--;
		slot = z_c7x_mmu_free_slot[z_c7x_mmu_free_count];
	} else {
		slot = z_c7x_mmu_get_next_slot();
		if (slot >= C7X_MMU_POOL_TABLES) {
			return NULL;
		}
		z_c7x_mmu_set_next_slot(slot + 1U);
	}

	base = z_c7x_mmu_get_tables_base() + (uint32_t)(slot * C7X_MMU_ENTRIES);
	for (i = 0U; i < C7X_MMU_ENTRIES; i++) {
		base[i] = 0U;
	}
	z_c7x_mmu_used[slot] = 0U;
	return base;
}

static void z_c7x_mmu_free_table(uint32_t slot)
{
	z_c7x_mmu_free_slot[z_c7x_mmu_free_count] = (uint8_t)slot;
	z_c7x_mmu_free_count++;
}

static void z_c7x_mmu_write_entry(uint64_t *table, uint32_t idx, uint64_t desc)
{
	uint32_t slot = z_c7x_mmu_slot(table);
	bool was = (table[idx] & C7X_MMU_DESC_TYPE_MASK) != C7X_MMU_DESC_INVALID;
	bool now = (desc & C7X_MMU_DESC_TYPE_MASK) != C7X_MMU_DESC_INVALID;

	table[idx] = desc;
	if (now && !was) {
		z_c7x_mmu_used[slot]++;
	} else if (was && !now) {
		z_c7x_mmu_used[slot]--;
	}
}

static uint64_t z_c7x_mmu_read_entry(const uint64_t *table, uint32_t idx)
{
	return table[idx];
}

static uint32_t z_c7x_mmu_index(uint64_t va, unsigned int level)
{
	return (uint32_t)((va >> C7X_MMU_LEVEL_SHIFT(level)) & C7X_MMU_INDEX_MASK);
}

static volatile uint32_t z_c7x_mmu_fail_stat[2] __used
	__aligned(64) Z_GENERIC_SECTION(.bss:c7x_diag);

static FUNC_NORETURN void z_c7x_mmu_fail(uint32_t code, uint64_t va)
{
	z_c7x_mmu_fail_stat[0] = 0xBAD00000U | (code & 0xFFFFU);
	z_c7x_mmu_fail_stat[1] = (uint32_t)va;
	for (;;) {
	}
}

static uint64_t *z_c7x_mmu_next_table(uint64_t *table, uint32_t idx)
{
	uint64_t e = z_c7x_mmu_read_entry(table, idx);
	uint64_t *child;

	if ((e & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_TABLE) {
		return (uint64_t *)(uintptr_t)(e & ~C7X_MMU_PAGE_MASK);
	}
	if ((e & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_BLOCK) {
		return NULL;
	}
	child = z_c7x_mmu_alloc_table();
	if (child == NULL) {
		return NULL;
	}
	z_c7x_mmu_write_entry(table, idx, (uint64_t)(uintptr_t)child | C7X_MMU_DESC_TABLE);
	return child;
}

static uint64_t *z_c7x_mmu_l2_table(uint64_t *l0, uint64_t va)
{
	uint64_t *l1 = z_c7x_mmu_next_table(l0, z_c7x_mmu_index(va, 0U));

	return (l1 == NULL) ? NULL : z_c7x_mmu_next_table(l1, z_c7x_mmu_index(va, 1U));
}

/* L3 table under an L2 entry, split from a block if one is there */
static uint64_t *z_c7x_mmu_l3_table(uint64_t *l2, uint32_t i2)
{
	uint64_t e = z_c7x_mmu_read_entry(l2, i2);
	uint64_t *l3;
	uint32_t k;

	if ((e & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_TABLE) {
		return (uint64_t *)(uintptr_t)(e & ~C7X_MMU_PAGE_MASK);
	}

	l3 = z_c7x_mmu_alloc_table();
	if (l3 == NULL) {
		return NULL;
	}
	if ((e & C7X_MMU_DESC_TYPE_MASK) != 0ULL) {
		uint64_t old_attr = e & C7X_MMU_PAGE_MASK;
		uint64_t old_pa = e & ~C7X_MMU_PAGE_MASK & ~C7X_MMU_BLOCK_MASK;
		uint64_t old_hi = e & C7X_MMU_UPPER_ATTRS;

		for (k = 0U; k < C7X_MMU_ENTRIES; k++) {
			z_c7x_mmu_write_entry(l3, k,
					      (old_pa + ((uint64_t)k << C7X_MMU_PAGE_SHIFT)) |
						      PAGE_OF_BLK(old_attr) | old_hi);
		}
	}
	z_c7x_mmu_write_entry(l2, i2, (uint64_t)(uintptr_t)l3 | C7X_MMU_DESC_TABLE);
	return l3;
}

static int z_c7x_mmu_map_block(uint64_t *l0, uint64_t va, uint64_t pa, uint64_t attr)
{
	uint64_t *l2 = z_c7x_mmu_l2_table(l0, va);
	uint32_t i2 = z_c7x_mmu_index(va, 2U);
	uint64_t old;

	if (l2 == NULL) {
		return -ENOMEM;
	}
	old = z_c7x_mmu_read_entry(l2, i2);
	z_c7x_mmu_write_entry(l2, i2, (pa & ~C7X_MMU_BLOCK_MASK) | attr);
	if ((old & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_TABLE) {
		z_c7x_mmu_free_table(
			z_c7x_mmu_slot((uint64_t *)(uintptr_t)(old & ~C7X_MMU_PAGE_MASK)));
	}
	return 0;
}

/* attr holds block descriptor attributes */
static int z_c7x_mmu_map_page(uint64_t *l0, uint64_t va, uint64_t pa, uint64_t attr)
{
	uint64_t *l2, *l3;

	l2 = z_c7x_mmu_l2_table(l0, va);
	if (l2 == NULL) {
		return -ENOMEM;
	}
	l3 = z_c7x_mmu_l3_table(l2, z_c7x_mmu_index(va, 2U));
	if (l3 == NULL) {
		return -ENOMEM;
	}
	z_c7x_mmu_write_entry(l3, z_c7x_mmu_index(va, 3U),
			      (pa & ~C7X_MMU_PAGE_MASK) | PAGE_OF_BLK(attr));
	return 0;
}

static void z_c7x_mmu_map(uint64_t *l0, uint64_t va, uint64_t pa, uint64_t size, uint32_t attr_idx)
{
	uint64_t attr = BLK_BASE | C7X_MMU_ATTR_IDX(attr_idx & C7X_MT_MASK);
	uint64_t step;
	int rc;

	if (((va | pa | size) & C7X_MMU_PAGE_MASK) != 0ULL) {
		z_c7x_mmu_fail(4U, va);
	}

	while (size != 0ULL) {
		if ((size >= C7X_MMU_BLOCK_SIZE) && (((va | pa) & C7X_MMU_BLOCK_MASK) == 0ULL)) {
			rc = z_c7x_mmu_map_block(l0, va, pa, attr);
			step = C7X_MMU_BLOCK_SIZE;
		} else {
			rc = z_c7x_mmu_map_page(l0, va, pa, attr);
			step = C7X_MMU_PAGE_SIZE;
		}
		if (rc != 0) {
			z_c7x_mmu_fail(2U, va);
		}
		va += step;
		pa += step;
		size -= step;
	}
}

/* The bits set in SCR to enable the MMU */
#define C7X_SCR_MMU_ENABLE 0x80000000000000C1ULL

void z_c7x_mmu_enable(void)
{
	z_c7x_write_scr(z_c7x_read_scr() | C7X_SCR_MMU_ENABLE);
}

void z_c7x_mmu_init(void)
{
	const struct c7x_mmu_region *r;
	uint64_t *l0;
	unsigned int i;

	z_c7x_mmu_mair_set(0U, C7X_MAIR0);
	z_c7x_mmu_mair_set(1U, C7X_MAIR1);
	z_c7x_mmu_mair_set(2U, C7X_MAIR2);
	z_c7x_mmu_mair_set(3U, C7X_MAIR3);
	z_c7x_mmu_mair_set(4U, C7X_MAIR4);
	z_c7x_mmu_mair_set(5U, C7X_MAIR5);
	z_c7x_mmu_mair_set(6U, C7X_MAIR6);
	z_c7x_mmu_mair_set(7U, C7X_MAIR7);

	/* MMU tables reside in both write-back and cacheable memory */
	z_c7x_mmu_tcr_set(C7X_TCR_ADDR_BITS(48U) | C7X_TCR_WALK_EN | C7X_TCR_MEMTYPE_CACHEABLE |
			  C7X_TCR_OUTER_CACHEABLE | C7X_TCR_INNER_CACHEABLE |
			  C7X_TCR_OUTER_SHAREABLE);

	z_c7x_mmu_set_next_slot(0U);
	z_c7x_mmu_free_count = 0U;
	l0 = z_c7x_mmu_alloc_table();
	for (i = 0U; i < mmu_config.num_regions; i++) {
		r = &mmu_config.mmu_regions[i];
		z_c7x_mmu_map(l0, (uint64_t)r->base_va, (uint64_t)r->base_pa, (uint64_t)r->size,
			      r->attrs);
	}

	z_c7x_mmu_tbr0_set(l0);

	z_c7x_mmu_enable();
}

/* No userspace support */
#define C7X_MMU_FLAGS_UNSUPPORTED K_MEM_PERM_USER

static int z_c7x_mmu_attr_idx(uint32_t flags, uint32_t *attr_idx)
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

/* Unlink the tables left with no valid entry on the walk to va */
static void z_c7x_mmu_prune(uint64_t *l0, uint64_t va, struct z_c7x_mmu_release *rel)
{
	uint64_t *table[C7X_MMU_LEVELS];
	unsigned int level;
	uint64_t e;

	table[0] = l0;
	for (level = 0U; level < C7X_MMU_LAST_LEVEL; level++) {
		e = z_c7x_mmu_read_entry(table[level], z_c7x_mmu_index(va, level));
		if ((e & C7X_MMU_DESC_TYPE_MASK) != C7X_MMU_DESC_TABLE) {
			break;
		}
		table[level + 1U] = (uint64_t *)(uintptr_t)(e & ~C7X_MMU_PAGE_MASK);
	}
	while (level > 0U && z_c7x_mmu_used[z_c7x_mmu_slot(table[level])] == 0U) {
		z_c7x_mmu_write_entry(table[level - 1U], z_c7x_mmu_index(va, level - 1U),
				      C7X_MMU_DESC_INVALID);
		rel->slot[rel->count] = (uint8_t)z_c7x_mmu_slot(table[level]);
		rel->count++;
		level--;
	}
}

static void z_c7x_mmu_release(struct z_c7x_mmu_release *rel)
{
	uint32_t i;

	for (i = 0U; i < rel->count; i++) {
		z_c7x_mmu_free_table(rel->slot[i]);
	}
}

static int z_c7x_mmu_unmap_page(uint64_t *l0, uint64_t va, struct z_c7x_mmu_release *rel)
{
	uint64_t *table = l0;
	unsigned int level;
	uint64_t e;

	for (level = 0U; level < C7X_MMU_LAST_LEVEL - 1U; level++) {
		e = z_c7x_mmu_read_entry(table, z_c7x_mmu_index(va, level));
		if ((e & C7X_MMU_DESC_TYPE_MASK) != C7X_MMU_DESC_TABLE) {
			return 0;
		}
		table = (uint64_t *)(uintptr_t)(e & ~C7X_MMU_PAGE_MASK);
	}
	e = z_c7x_mmu_read_entry(table, z_c7x_mmu_index(va, level));
	if ((e & C7X_MMU_DESC_TYPE_MASK) == C7X_MMU_DESC_INVALID) {
		return 0;
	}
	table = z_c7x_mmu_l3_table(table, z_c7x_mmu_index(va, level));
	if (table == NULL) {
		return -ENOMEM;
	}
	z_c7x_mmu_write_entry(table, z_c7x_mmu_index(va, C7X_MMU_LAST_LEVEL), C7X_MMU_DESC_INVALID);
	z_c7x_mmu_prune(l0, va, rel);
	return 0;
}

static int z_c7x_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	struct z_c7x_mmu_release rel = {.count = 0U};
	uintptr_t va = (uintptr_t)virt;
	uint32_t attr_idx;
	uint64_t attr;
	uint64_t *l0;
	size_t off;
	unsigned int key;
	int rc;

	rc = z_c7x_mmu_attr_idx(flags, &attr_idx);
	if (rc != 0) {
		return rc;
	}
	if (((va | phys | (uintptr_t)size) & C7X_MMU_PAGE_MASK) != 0U) {
		return -EINVAL;
	}
	if (size == 0U) {
		return 0;
	}
	attr = BLK_BASE | C7X_MMU_ATTR_IDX(attr_idx);
	if ((flags & K_MEM_PERM_RW) == 0U) {
		attr |= C7X_MMU_AP_PRO;
	}
	if ((flags & K_MEM_PERM_EXEC) == 0U) {
		attr |= C7X_MMU_PXN;
	}

	key = irq_lock();
	l0 = z_c7x_mmu_get_tables_base();
	for (off = 0U; off < size; off += C7X_MMU_PAGE_SIZE) {
		rc = z_c7x_mmu_map_page(l0, (uint64_t)(va + off), (uint64_t)(phys + off), attr);
		if (rc != 0) {
			z_c7x_mmu_prune(l0, (uint64_t)(va + off), &rel);
			break;
		}
	}
	z_c7x_mmu_tlb_inv(C7X_TLB_INV_ALL);
	z_c7x_mmu_release(&rel);
	irq_unlock(key);
	return rc;
}

static int z_c7x_mem_unmap(void *addr, size_t size)
{
	struct z_c7x_mmu_release rel = {.count = 0U};
	uintptr_t va = (uintptr_t)addr;
	uint64_t *l0;
	size_t off;
	unsigned int key;
	int rc = 0;

	if (((va | (uintptr_t)size) & C7X_MMU_PAGE_MASK) != 0U) {
		return -EINVAL;
	}
	if (size == 0U) {
		return 0;
	}

	key = irq_lock();
	l0 = z_c7x_mmu_get_tables_base();
	for (off = 0U; off < size; off += C7X_MMU_PAGE_SIZE) {
		rc = z_c7x_mmu_unmap_page(l0, (uint64_t)(va + off), &rel);
		if (rc != 0) {
			break;
		}
	}
	z_c7x_mmu_tlb_inv(C7X_TLB_INV_ALL);
	z_c7x_mmu_release(&rel);
	irq_unlock(key);
	return rc;
}

int arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
	int rc = z_c7x_mem_map(virt, phys, size, flags);

	if (rc != 0) {
		LOG_ERR("map %p <- 0x%lx size 0x%zx: %d", virt, (unsigned long)phys, size, rc);
	}
	return rc;
}

int arch_mem_unmap(void *addr, size_t size)
{
	return z_c7x_mem_unmap(addr, size);
}

int arch_page_phys_get(void *virt, uintptr_t *phys)
{
	uint64_t va = (uint64_t)(uintptr_t)virt;
	uint64_t *table = z_c7x_mmu_get_tables_base();
	uint64_t e = 0U;
	unsigned int level;

	for (level = 0U; level < C7X_MMU_LEVELS; level++) {
		uint32_t idx = z_c7x_mmu_index(va, level);
		uint64_t out_mask = BIT64_MASK(C7X_MMU_LEVEL_SHIFT(level));

		e = z_c7x_mmu_read_entry(table, idx);
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
		table = (uint64_t *)(uintptr_t)(e & ~C7X_MMU_UPPER_ATTRS & ~C7X_MMU_PAGE_MASK);
	}
	return -EFAULT;
}
