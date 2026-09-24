/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MBOX_MBOX_TI_OMAP_C7X_H_
#define ZEPHYR_INCLUDE_DRIVERS_MBOX_MBOX_TI_OMAP_C7X_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mbox.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Take one message from a receive channel without using the interrupt.
 *
 * Usable with interrupts locked and without a running scheduler.
 *
 * @param dev Mailbox device.
 * @param channel Receive channel.
 * @param value Message read from the channel.
 *
 * @retval 0 A message was read into @p value.
 * @retval -ENODATA The channel is empty.
 * @retval -ENOSYS @p channel is not a receive channel.
 */
int mbox_ti_omap_c7x_poll(const struct device *dev, mbox_channel_id_t channel, uint32_t *value);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MBOX_MBOX_TI_OMAP_C7X_H_ */
