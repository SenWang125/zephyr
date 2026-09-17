/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 *
 *  OMAP mailbox on cluster 1 at 0x29010000: fifo0 is C7x->A53, fifo1 is A53->C7x.
 *  The register layout is the standard OMAP one (MESSAGE +0x40, FIFO_STATUS +0x80,
 *  MSG_STATUS +0xC0, INT ENABLE/CLEAR/DISABLE +0x108/104/10C per user, EOI +0x140).
 *  On the C7x the mailbox interrupt arrives through the CLEC. Input event 193 to
 *  C7x local interrupt 60.
 */

/* The MESSAGE register the send path writes. */
#define MAILBOX_MBOX_SIZE	sizeof(uint32_t)

#define DT_DRV_COMPAT ti_omap_mailbox_c7x

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/sys/sys_io.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/interrupt_controller/intc_ti_c7x_clec.h>

LOG_MODULE_REGISTER(mbox_omap_c7x, CONFIG_MBOX_LOG_LEVEL);

/* OMAP mailbox register offsets. */
#define MBOX_MESSAGE(base, f)     ((base) + 0x40u + 0x4u * (f))
#define MBOX_FIFOSTATUS(base, f)  ((base) + 0x80u + 0x4u * (f))
#define MBOX_MSGSTATUS(base, f)   ((base) + 0xC0u + 0x4u * (f))
#define MBOX_MSGSTATUS_COUNT      0x7u

#define FIFO_TX   0u
#define FIFO_RX   1u

#define MBOX_USER_C7X             1u
#define MBOX_IRQSTATUS_CLR(b, u)  ((b) + 0x104u + 0x10u * (u))
#define MBOX_IRQENABLE_SET(b, u)  ((b) + 0x108u + 0x10u * (u))
#define MBOX_IRQENABLE_CLR(b, u)  ((b) + 0x10Cu + 0x10u * (u))
#define MBOX_NEWMSG(f)            (1u << (2u * (f)))

#define CH_TX     0u
#define CH_RX     1u

#define MBOX_TX_SPIN_LIMIT  200000u

struct mbox_omap_c7x_config {
	uintptr_t base;
	unsigned int irqn;
	unsigned int irq_prio;
};

struct mbox_omap_c7x_data {
	mbox_callback_t cb;
	void           *user_data;
	bool            rx_enabled;
	bool            rx_armed;
};

static void mbox_omap_c7x_drain(const struct device *dev)
{
	const struct mbox_omap_c7x_config *cfg = dev->config;
	struct mbox_omap_c7x_data *data = dev->data;
	uint32_t value;
	unsigned int key;
	struct mbox_msg msg = {
		.data = &value,
		.size = MAILBOX_MBOX_SIZE,
	};

	for (;;) {
		key = arch_irq_lock();
		if ((sys_read32(MBOX_MSGSTATUS(cfg->base, FIFO_RX)) &
		     MBOX_MSGSTATUS_COUNT) == 0u) {
			arch_irq_unlock(key);
			return;
		}
		value = sys_read32(MBOX_MESSAGE(cfg->base, FIFO_RX));
		arch_irq_unlock(key);

		if (data->rx_enabled && data->cb != NULL) {
			(void)sys_cache_data_flush_and_invd_all();
			data->cb(dev, CH_RX, data->user_data, &msg);
		}
	}
}

static void mbox_omap_c7x_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct mbox_omap_c7x_config *cfg = dev->config;

	sys_write32(MBOX_NEWMSG(FIFO_RX), MBOX_IRQSTATUS_CLR(cfg->base, MBOX_USER_C7X));
	mbox_omap_c7x_drain(dev);
}

static int mbox_omap_c7x_send(const struct device *dev, mbox_channel_id_t ch,
			      const struct mbox_msg *msg)
{
	unsigned int key;
	const struct mbox_omap_c7x_config *cfg = dev->config;
	uint32_t data32 = 0;

	if (ch != CH_TX) {
		return -ENOSYS;
	}
	/* Same contract as mbox_ti_omap.c: a NULL message is a doorbell carrying
	 * 0, which the host reads as its first vring's notify id.
	 */
	if (msg != NULL) {
		if (msg->size > sizeof(data32)) {
			return -EMSGSIZE;
		}
		memcpy(&data32, msg->data, msg->size);
	}
	(void)sys_cache_data_flush_and_invd_all();
	/* test and store as one unit, as IpcNotify_sendMsg does. The ISR's ack path also writes */
	key = arch_irq_lock();
	for (uint32_t i = 0; i < MBOX_TX_SPIN_LIMIT; i++) {
		if ((sys_read32(MBOX_FIFOSTATUS(cfg->base, FIFO_TX)) & 0x1u) == 0u) {
			sys_write32(data32, MBOX_MESSAGE(cfg->base, FIFO_TX));
			arch_irq_unlock(key);
			return 0;
		}
	}
	arch_irq_unlock(key);
	return -EAGAIN;
}

static int mbox_omap_c7x_register_callback(const struct device *dev,
					   mbox_channel_id_t ch,
					   mbox_callback_t cb, void *user_data)
{
	struct mbox_omap_c7x_data *data = dev->data;

	if (ch != CH_RX) {
		return -ENOSYS;
	}
	data->cb = cb;
	data->user_data = user_data;
	return 0;
}

static int mbox_omap_c7x_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MAILBOX_MBOX_SIZE;
}

static uint32_t mbox_omap_c7x_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 2u;
}

static int mbox_omap_c7x_set_enabled(const struct device *dev,
				     mbox_channel_id_t ch, bool enable)
{
	struct mbox_omap_c7x_data *data = dev->data;

	if (ch != CH_RX) {
		return -ENOSYS;
	}
	data->rx_enabled = enable;
	if (enable && !data->rx_armed) {
		const struct mbox_omap_c7x_config *cfg = dev->config;

		/* the enable survives a DSP-only restart. IpcNotify_init masks before it arms */
		sys_write32(MBOX_NEWMSG(FIFO_RX), MBOX_IRQENABLE_CLR(cfg->base, MBOX_USER_C7X));
		irq_connect_dynamic(cfg->irqn, cfg->irq_prio, mbox_omap_c7x_isr, dev, 0U);
		c7x_clec_irq_enable(cfg->irqn);
		sys_write32(MBOX_NEWMSG(FIFO_RX), MBOX_IRQSTATUS_CLR(cfg->base, MBOX_USER_C7X));
		sys_write32(MBOX_NEWMSG(FIFO_RX), MBOX_IRQENABLE_SET(cfg->base, MBOX_USER_C7X));
		irq_enable(cfg->irqn);
		data->rx_armed = true;
		mbox_omap_c7x_drain(dev);
	}
	return 0;
}

static DEVICE_API(mbox, mbox_omap_c7x_api) = {
	.send = mbox_omap_c7x_send,
	.register_callback = mbox_omap_c7x_register_callback,
	.mtu_get = mbox_omap_c7x_mtu_get,
	.max_channels_get = mbox_omap_c7x_max_channels_get,
	.set_enabled = mbox_omap_c7x_set_enabled,
};

static int mbox_omap_c7x_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

#define MBOX_OMAP_C7X_INIT(idx)                                                \
	static const struct mbox_omap_c7x_config mbox_omap_c7x_cfg_##idx = {    \
		.base = DT_INST_REG_ADDR(idx),                                 \
		.irqn = DT_INST_IRQN(idx),                                     \
		.irq_prio = DT_INST_IRQ(idx, priority),                        \
	};                                                                     \
	static struct mbox_omap_c7x_data mbox_omap_c7x_data_##idx;             \
	DEVICE_DT_INST_DEFINE(idx, mbox_omap_c7x_init, NULL,                    \
			      &mbox_omap_c7x_data_##idx,                       \
			      &mbox_omap_c7x_cfg_##idx, POST_KERNEL,           \
			      CONFIG_MBOX_INIT_PRIORITY, &mbox_omap_c7x_api);

DT_INST_FOREACH_STATUS_OKAY(MBOX_OMAP_C7X_INIT)
