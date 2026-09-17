/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_C7X_INCLUDE_C7X_MMU_TABLES_H_
#define ZEPHYR_ARCH_C7X_INCLUDE_C7X_MMU_TABLES_H_

#include <stdint.h>

extern uint32_t c7x_mmu_tables[];
extern uint32_t c7x_mmu_next_slot;

uint32_t c7x_mmu_get_next_slot(void);
void c7x_mmu_set_next_slot(uint32_t slot);
uint32_t *c7x_mmu_alloc_table(void);
void c7x_mmu_write_entry(uint32_t *table, uint32_t idx, uint64_t desc);
uint64_t c7x_mmu_read_entry(const uint32_t *table, uint32_t idx);
void c7x_mmu_map(uint32_t *l0, uint64_t va, uint64_t pa, uint64_t size, uint32_t attr_idx);

#endif /* ZEPHYR_ARCH_C7X_INCLUDE_C7X_MMU_TABLES_H_ */
