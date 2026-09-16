/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/ztest.h>

static int fake_send(const struct device *dev, mbox_channel_id_t channel,
		     const struct mbox_msg *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(msg);

	return 0;
}

static DEVICE_API(mbox, fake_api) = {
	.send = fake_send,
};

DEVICE_DEFINE(fake_mbox, "fake_mbox", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fake_api);

/* The return type is unsigned, so an error code would read as a huge channel count. */
ZTEST(mbox_no_max_channels, test_no_getter_reports_no_channels)
{
	const struct device *dev = DEVICE_GET(fake_mbox);

	zassert_equal(mbox_max_channels_get(dev), 0U);
}

ZTEST_SUITE(mbox_no_max_channels, NULL, NULL, NULL, NULL, NULL);
