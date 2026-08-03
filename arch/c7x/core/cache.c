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
 * BLKCMO threshold. Below it __DCCIC (flush and invalidate) over this fixed block,
 * at or above it __DCCMIC over the requested size.
 */
#define C7X_CACHE_SMALL_RANGE 1280U

void z_c7x_cache_wait(void)
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
	z_c7x_cache_wait();
	irq_unlock(key);
	return 0;
}

/* BLKCMO has no flush-only operation, so a range flush also invalidates */
int arch_dcache_flush_range(void *addr, size_t size)
{
	unsigned int key = irq_lock();

	__se_cache_op(addr, __DCCIC, MAX(C7X_CACHE_SMALL_RANGE, size));
	z_c7x_cache_wait();
	irq_unlock(key);
	return 0;
}

int arch_dcache_flush_and_invd_range(void *addr, size_t size)
{
	return arch_dcache_flush_range(addr, size);
}

int arch_dcache_flush_all(void)
{
	/* writeback only (set L1DWB, ECR value of 258) */
	z_c7x_write_l1dwb(1U);
	barrier_dmem_fence_full();
	return 0;
}

int arch_dcache_invd_all(void)
{
	/* invalidate only (set L1DINV, ECR value of 260) */
	z_c7x_write_l1dinv(1U);
	barrier_dmem_fence_full();
	return 0;
}

int arch_dcache_flush_and_invd_all(void)
{
	/* writeback then invalidate (set L1DWBINV, ECR value of 259) */
	z_c7x_write_l1dwbinv(1U);
	barrier_dmem_fence_full();
	return 0;
}

void z_c7x_l1d_enable(void)
{
	uint64_t cfg = z_c7x_l1dcfg_get() | C7X_L1DCFG_ON;

	if (IS_ENABLED(CONFIG_C7X_L1D_WRITE_THROUGH)) {
		if ((cfg & C7X_L1DCFG_WB) != 0U) {
			arch_dcache_flush_and_invd_all();
		}
		cfg &= ~C7X_L1DCFG_WB;
	} else {
		cfg |= C7X_L1DCFG_WB;
	}
	z_c7x_l1dcfg_set(cfg);
}

void arch_dcache_enable(void)
{
	z_c7x_l1d_enable();
}

void arch_dcache_disable(void)
{
	unsigned int key = irq_lock();

	/* Flush dirty lines and invalidate to keep l1d fresh when comes to enable */
	arch_dcache_flush_and_invd_all();
	z_c7x_l1dcfg_set(z_c7x_l1dcfg_get() & ~C7X_L1DCFG_ON);
	irq_unlock(key);
}
