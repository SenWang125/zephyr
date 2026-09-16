/*
 *  Copyright (c) 2026 Texas Instruments Incorporated
 *  SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_C7X_INCLUDE_C7X_MMU_TABLES_H_
#define ZEPHYR_ARCH_C7X_INCLUDE_C7X_MMU_TABLES_H_

#include <stdint.h>

extern uint32_t mmu_tables[];
extern uint32_t mmu_next_slot;

uint32_t mmu_get_next_slot(void);
void mmu_set_next_slot(uint32_t slot);
uint32_t *mmu_alloc_table(void);
void mmu_write_entry(uint32_t *table, uint32_t idx, uint64_t desc);
uint64_t mmu_read_entry(const uint32_t *table, uint32_t idx);
void mmu_map(uint32_t *l0, uint64_t va, uint64_t pa, uint64_t size, uint32_t attr_idx);

/* per-region attribute indexes */

#endif /* ZEPHYR_ARCH_C7X_INCLUDE_C7X_MMU_TABLES_H_ */
