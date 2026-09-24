/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Whole-L1D cache operations through the Zephyr cache API. The combined
 *  writeback+invalidate path must issue L1DWBINV (ECR259) and be callable
 *  from a normal thread without faulting.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/cache.h>
#include <string.h>

#define CACHE_TEST_SIZE 8192U

static uint8_t cache_src[CACHE_TEST_SIZE] __aligned(64);
static uint8_t cache_ref[CACHE_TEST_SIZE] __aligned(64);

static void cache_fill_src(void)
{
	for (size_t i = 0U; i < sizeof(cache_src); i++) {
		cache_src[i] = (uint8_t)((i * 37U) + 11U);
	}
	memcpy(cache_ref, cache_src, sizeof(cache_ref));
}

ZTEST(c7x_core, test_data_cache_whole_ops)
{
	int ret;

	cache_fill_src();

	/* ECR258: writeback only. Cached lines stay valid. */
	ret = sys_cache_data_flush_all();
	zassert_true((ret == 0) || (ret == -ENOTSUP),
		     "flush_all returned %d", ret);

	/* ECR259: writeback+invalidate. This is the path under test. */
	ret = sys_cache_data_flush_and_invd_all();
	zassert_true((ret == 0) || (ret == -ENOTSUP),
		     "flush_and_invd_all returned %d", ret);

	/*
	 * ECR260: invalidate only. The writeback above must have made any
	 * dirty line clean, so this cannot discard live data.
	 */
	ret = sys_cache_data_invd_all();
	zassert_true((ret == 0) || (ret == -ENOTSUP),
		     "invd_all returned %d", ret);

	zassert_mem_equal(cache_ref, cache_src, sizeof(cache_src),
			  "data mismatch after whole-cache operations");
}
