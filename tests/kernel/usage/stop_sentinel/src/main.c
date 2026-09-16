/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <usage.h>

/* a query while usage0 is 0 must not charge the whole uptime */

static uint32_t uptime_cycles(void)
{
	k_msleep(100);
	return k_cycle_get_32();
}

ZTEST(usage_stop_sentinel, test_thread_stats_while_stopped)
{
	k_thread_runtime_stats_t before, during;
	k_tid_t self = k_current_get();
	uint32_t uptime = uptime_cycles();

	zassert_true(uptime > 0U);
	zassert_ok(k_thread_runtime_stats_get(self, &before));
	z_sched_usage_stop();
	zassert_ok(k_thread_runtime_stats_get(self, &during));
	z_sched_usage_start(self);

	zassert_true(during.execution_cycles - before.execution_cycles < uptime / 2U,
		     "charged %llu cycles with %u since boot",
		     during.execution_cycles - before.execution_cycles, uptime);
}

ZTEST(usage_stop_sentinel, test_cpu_stats_while_stopped)
{
	k_thread_runtime_stats_t before, during;
	uint32_t uptime = uptime_cycles();

	zassert_ok(k_thread_runtime_stats_all_get(&before));
	z_sched_usage_stop();
	zassert_ok(k_thread_runtime_stats_all_get(&during));
	z_sched_usage_start(k_current_get());

	zassert_true(during.execution_cycles - before.execution_cycles < uptime / 2U,
		     "charged %llu cycles with %u since boot",
		     during.execution_cycles - before.execution_cycles, uptime);
}

ZTEST(usage_stop_sentinel, test_disable_while_stopped)
{
	k_thread_runtime_stats_t before, after;
	k_tid_t self = k_current_get();
	uint32_t uptime = uptime_cycles();

	zassert_ok(k_thread_runtime_stats_get(self, &before));
	z_sched_usage_stop();
	zassert_ok(k_thread_runtime_stats_disable(self));
	z_sched_usage_start(self);
	zassert_ok(k_thread_runtime_stats_get(self, &after));
	zassert_ok(k_thread_runtime_stats_enable(self));

	zassert_true(after.execution_cycles - before.execution_cycles < uptime / 2U,
		     "charged %llu cycles with %u since boot",
		     after.execution_cycles - before.execution_cycles, uptime);
}

ZTEST_SUITE(usage_stop_sentinel, NULL, NULL, NULL, NULL, NULL);
