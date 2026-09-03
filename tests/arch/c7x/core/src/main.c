/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/arch/c7x/lib_helpers.h>

extern char z_vecs_reset[];

ZTEST_SUITE(c7x_core, NULL, NULL, NULL, NULL, NULL);

static volatile uint32_t wake_ticks;
static K_SEM_DEFINE(wake_sem, 0, 1);

static void wake_tick(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	if (wake_ticks++ == 0U) {
		k_sem_give(&wake_sem);
	}
}

static K_TIMER_DEFINE(wake_timer, wake_tick, NULL);

ZTEST(c7x_core, test_thread_woken_from_isr_takes_events)
{
	wake_ticks = 0U;
	k_timer_start(&wake_timer, K_MSEC(1), K_MSEC(1));
	zassert_ok(k_sem_take(&wake_sem, K_MSEC(100)), "timer never fired");

	unsigned int cop = z_c7x_read_cop();
	uint32_t before = wake_ticks;

	k_busy_wait(20000);

	uint32_t during = wake_ticks - before;

	k_timer_stop(&wake_timer);
	zassert_equal(cop, C7X_TSR_COP_TASK_MODE, "woken thread runs at COP 0x%x", cop);
	zassert_true(during >= 10U, "%u timer events in a 20 ms busy wait", during);
}

static void check_busy_wait_ms(uint32_t ms)
{
	int64_t start = k_uptime_get();

	k_busy_wait(ms * USEC_PER_MSEC);

	int64_t took = k_uptime_get() - start;

	zassert_true(took >= ms - ms / 100U && took <= ms + ms / 100U + 2U,
		     "k_busy_wait of %u ms took %lld ms on the system timer", ms,
		     (long long)took);
}

ZTEST(c7x_core, test_busy_wait_against_the_system_timer)
{
	/* one second past the 32-bit range of the cycle counter */
	uint32_t past_32bit_ms = UINT32_MAX / (sys_clock_hw_cycles_per_sec() / MSEC_PER_SEC) + 1000U;

	check_busy_wait_ms(20U);
	check_busy_wait_ms(past_32bit_ms);
}

ZTEST(c7x_core, test_cxm_is_supervisor)
{
	unsigned int cxm = z_c7x_read_cxm();

	TC_PRINT("CXM=%u COP=0x%x\n", cxm, z_c7x_read_cop());
	zassert_true(C7X_CXM_IS_SUPERVISOR(cxm), "CXM %u does not service events", cxm);
}

ZTEST(c7x_core, test_dnum_is_the_devicetree_cpu)
{
	uint64_t dnum = z_c7x_read_dnum();

	zassert_equal(dnum & C7X_DNUM_CLUSTER_MASK, DT_REG_ADDR(DT_NODELABEL(cpu0)),
		      "DNUM 0x%llx is not the cpu0 reg", (unsigned long long)dnum);
}

ZTEST(c7x_core, test_estp_for_current_mode)
{
	uint64_t want = (uint64_t)(uintptr_t)z_vecs_reset;

	/* the bring-up programs the ESTP copy of the mode the core is in */
	switch (z_c7x_read_cxm()) {
	case C7X_CXM_S:
		zassert_equal(z_c7x_read_estp_s(), want, "ESTP_S is not the vector table");
		break;
	case C7X_CXM_GS:
		zassert_equal(z_c7x_read_estp_gs(), want, "ESTP_GS is not the vector table");
		break;
	default:
		zassert_true(false, "not a supervisor mode");
		break;
	}
}

#ifdef CONFIG_C7X_ILUT
ZTEST(c7x_core, test_ilut_enable_and_disable)
{
	uint64_t boot = __ILTER;
	uint64_t off;

	z_c7x_ilut_disable();
	off = __ILTER;
	z_c7x_ilut_enable();

	zassert_equal(boot, __ILUT_RW, "ILTER 0x%llx at boot", (unsigned long long)boot);
	zassert_equal(off, __ILUT_DISALLOWED, "ILTER 0x%llx after disable",
		      (unsigned long long)off);
	zassert_equal(__ILTER, __ILUT_RW, "ILTER 0x%llx after enable",
		      (unsigned long long)__ILTER);
}
#endif

BUILD_ASSERT(__builtin_clz(1U) == 31);
BUILD_ASSERT(__builtin_clz(0x80000000U) == 0);
BUILD_ASSERT(__builtin_clzll(1ULL << 40) == 23);
BUILD_ASSERT(LOG2(1000U) == 9);

/* The constant operand takes the folded path, the volatile copy the instruction. */
#define CHECK_CLZLL(c)                                                                             \
	do {                                                                                       \
		volatile unsigned long long v = (c);                                               \
		zassert_equal(__builtin_clzll(c), __builtin_clzll(v), "clzll of %s", #c);          \
	} while (false)

#define CHECK_CLZ(c)                                                                               \
	do {                                                                                       \
		volatile unsigned int v = (c);                                                     \
		zassert_equal(__builtin_clz(c), __builtin_clz(v), "clz of %s", #c);                \
	} while (false)

ZTEST(c7x_core, test_clz_constant_matches_runtime)
{
	CHECK_CLZ(1U);
	CHECK_CLZ(3U);
	CHECK_CLZ(1000U);
	CHECK_CLZ(0x80000000U);
	CHECK_CLZ(0xFFFFFFFFU);
	CHECK_CLZLL(1ULL);
	CHECK_CLZLL(1000ULL);
	CHECK_CLZLL(1ULL << 40);
	CHECK_CLZLL(0xFFFFFFFFFFFFFFFFULL);
}
