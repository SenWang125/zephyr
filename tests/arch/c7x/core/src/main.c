/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

ZTEST_SUITE(c7x_core, NULL, NULL, NULL, NULL, NULL);

ZTEST(c7x_core, test_irq_lock_nests)
{
	unsigned int outer = arch_irq_lock();
	unsigned int inner = arch_irq_lock();

	zassert_true(arch_irq_unlocked(outer), "interrupts were not enabled on entry");
	zassert_false(arch_irq_unlocked(inner), "nested lock did not read as locked");
	arch_irq_unlock(inner);
	arch_irq_unlock(outer);
}

ZTEST(c7x_core, test_cycle_counter_rate)
{
	const uint32_t us = 1000;
	uint32_t before = k_cycle_get_32();

	k_busy_wait(us);

	uint32_t elapsed = k_cycle_get_32() - before;
	uint32_t expect = (uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() * us / USEC_PER_SEC);

	zassert_true(elapsed >= expect, "%u cycles over %u us, expected at least %u",
		     elapsed, us, expect);
	zassert_true(elapsed < 4 * expect, "%u cycles over %u us, more than 4x the rate",
		     elapsed, us);
}

ZTEST(c7x_core, test_cycle_counter_64_matches_32)
{
	uint64_t c64 = k_cycle_get_64();
	uint32_t c32 = k_cycle_get_32();

	zassert_true((uint32_t)c64 <= c32, "32-bit counter is behind the 64-bit one");
	zassert_true(c32 - (uint32_t)c64 < sys_clock_hw_cycles_per_sec() / 100,
		     "the two counters read more than 10 ms apart");
}

ZTEST(c7x_core, test_tick_advances)
{
	int64_t t0 = k_uptime_ticks();

	k_sleep(K_MSEC(20));
	zassert_true(k_uptime_ticks() - t0 >= k_ms_to_ticks_floor64(20),
		     "uptime did not advance across a 20 ms sleep");
}
