/*
 *  SPDX-License-Identifier: Apache-2.0
 *  Copyright (c) 2026 Texas Instruments Incorporated
 *
 * volatile: each value must go through memory and must not be folded.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/c7x/mmu.h>
#include <c7x_mmu_tables.h>

uint32_t *c7x_mmu_l0_root = mmu_tables;

uint32_t *mmu_get_tables_base(void)
{
	return *(uint32_t *volatile *)&c7x_mmu_l0_root;
}

uint32_t mmu_get_next_slot(void)
{
	return *(volatile uint32_t *)&mmu_next_slot;
}

void mmu_set_next_slot(uint32_t slot)
{
	*(volatile uint32_t *)&mmu_next_slot = slot;
}
