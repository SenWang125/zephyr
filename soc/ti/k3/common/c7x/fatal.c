/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/sys/sys_io.h>

#include <kernel_arch_func.h>
#include <soc.h>

#define IPC_NODE     DT_CHOSEN(zephyr_ipc)
#define MBOX_RX_BASE DT_REG_ADDR(DT_MBOX_CTLR_BY_NAME(IPC_NODE, rx))
#define MBOX_RX_FIFO DT_MBOX_CHANNEL_BY_NAME(IPC_NODE, rx)

#define MBOX_MESSAGE(f)    (MBOX_RX_BASE + 0x40U + 4U * (f))
#define MBOX_MSG_STATUS(f) (MBOX_RX_BASE + 0xC0U + 4U * (f))
#define MBOX_MSG_COUNT     GENMASK(2, 0)

/* interrupts are locked here, so the receive FIFO is read directly */
static uint32_t mbox_rx_wait(void)
{
	while ((sys_read32(MBOX_MSG_STATUS(MBOX_RX_FIFO)) & MBOX_MSG_COUNT) == 0U) {
	}
	return sys_read32(MBOX_MESSAGE(MBOX_RX_FIFO));
}

void z_soc_system_halt_hook(void)
{
	const struct mbox_dt_spec tx = MBOX_DT_SPEC_GET(IPC_NODE, tx);
	uint32_t ack = TI_K3_RP_MBOX_SHUTDOWN_ACK;
	const struct mbox_msg msg = {
		.data = &ack,
		.size = sizeof(ack),
	};

	/* the host resets this core only once it has acknowledged the shutdown request */
	while (mbox_rx_wait() != TI_K3_RP_MBOX_SHUTDOWN) {
	}

	while (mbox_send_dt(&tx, &msg) == -EBUSY) {
	}
}
