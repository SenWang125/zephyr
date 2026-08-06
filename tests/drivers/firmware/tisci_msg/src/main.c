/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/ztest.h>

#include "tisci.h"

#define HDR_LEN sizeof(struct tisci_msg_hdr)

ZTEST(tisci_msg, test_clock_request_length)
{
	zassert_equal(HDR_LEN, 8);

	zassert_equal(sizeof(struct tisci_msg_req_set_clock_state), HDR_LEN + 10);
	zassert_equal(sizeof(struct tisci_msg_req_get_clock_state), HDR_LEN + 9);
	zassert_equal(sizeof(struct tisci_msg_req_set_clock_parent), HDR_LEN + 14);
	zassert_equal(sizeof(struct tisci_msg_req_get_clock_parent), HDR_LEN + 9);
	zassert_equal(sizeof(struct tisci_msg_req_get_clock_num_parents), HDR_LEN + 9);
	zassert_equal(sizeof(struct tisci_msg_req_query_clock_freq), HDR_LEN + 33);
	zassert_equal(sizeof(struct tisci_msg_req_set_clock_freq), HDR_LEN + 33);
	zassert_equal(sizeof(struct tisci_msg_req_get_clock_freq), HDR_LEN + 9);
}

ZTEST(tisci_msg, test_clock_request_id_32_offset)
{
	zassert_equal(offsetof(struct tisci_msg_req_set_clock_state, clk_id_32), HDR_LEN + 6);
	zassert_equal(offsetof(struct tisci_msg_req_get_clock_state, clk_id_32), HDR_LEN + 5);
	zassert_equal(offsetof(struct tisci_msg_req_set_clock_parent, clk_id_32), HDR_LEN + 6);
	zassert_equal(offsetof(struct tisci_msg_req_set_clock_parent, parent_id_32), HDR_LEN + 10);
	zassert_equal(offsetof(struct tisci_msg_req_get_clock_parent, clk_id_32), HDR_LEN + 5);
	zassert_equal(offsetof(struct tisci_msg_req_get_clock_num_parents, clk_id_32), HDR_LEN + 5);
	zassert_equal(offsetof(struct tisci_msg_req_query_clock_freq, clk_id_32), HDR_LEN + 29);
	zassert_equal(offsetof(struct tisci_msg_req_set_clock_freq, clk_id_32), HDR_LEN + 29);
	zassert_equal(offsetof(struct tisci_msg_req_get_clock_freq, clk_id_32), HDR_LEN + 5);
}

ZTEST_SUITE(tisci_msg, NULL, NULL, NULL, NULL, NULL);
