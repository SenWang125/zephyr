/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/arch/c7x/cache.h>
#include <zephyr/arch/c7x/cpu.h>
#include <c7x.h>

/*
 * BLKCMO threshold. Below it __DCCIC (clean and invalidate) on a fixed block,
 * at or above it __DCCMIC over the requested size.
 */
#define C7X_CACHE_SMALL_RANGE  1280U

static inline void c7x_cache_wait(void)
{
	__SE0ADV(char);
	barrier_dmem_fence_full();
}

int arch_dcache_invd_range(void *addr, size_t size)
{
	unsigned int key = irq_lock();

	if (size < C7X_CACHE_SMALL_RANGE) {
		__se_cache_op(addr, __DCCIC, C7X_CACHE_SMALL_RANGE);
	} else {
		__se_cache_op(addr, __DCCMIC, size);
	}
	c7x_cache_wait();
	irq_unlock(key);
	return 0;
}

int arch_dcache_flush_range(void *addr, size_t size)
{
	unsigned int key = irq_lock();

	__se_cache_op(addr, __DCCIC,
		      (size < C7X_CACHE_SMALL_RANGE) ? C7X_CACHE_SMALL_RANGE : size);
	c7x_cache_wait();
	irq_unlock(key);
	return 0;
}

int arch_dcache_flush_and_invd_range(void *addr, size_t size)
{
	return arch_dcache_flush_range(addr, size);
}

__noinline int arch_dcache_flush_all(void)
{
	/* writeback without invalidate (set L1DWB, ECR258) */
	__asm__ volatile (" MVKU32 .S1 1, A1\n"
			  " NOP 4\n"
			  " MVC .S1 A1, L1DWB\n"
			  " NOP 8\n");
	return 0;
}

__noinline int arch_dcache_invd_all(void)
{
	/* invalidate without writeback (set L1DINV, ECR260) */
	__asm__ volatile (" MVKU32 .S1 1, A1\n"
			  " NOP 4\n"
			  " MVC .S1 A1, L1DINV\n"
			  " NOP 8\n");
	return 0;
}

int arch_dcache_flush_and_invd_all(void)
{
	c7x_l1d_wbinv(C7X_L1D_WBINV_ALL);
	return 0;
}

void c7x_l1d_enable_wt(void)
{
	uint64_t cfg = c7x_l1dcfg_get();

	cfg = (cfg | C7X_L1DCFG_ON) & ~C7X_L1DCFG_WB;
	c7x_l1dcfg_set(cfg);
	(void)arch_dcache_invd_all();
}

/* write-through is the only mode L1D supports */
void arch_dcache_enable(void)
{
	c7x_l1d_enable_wt();
}

void arch_dcache_disable(void)
{
	c7x_l1dcfg_set(c7x_l1dcfg_get() & ~C7X_L1DCFG_ON);
}
