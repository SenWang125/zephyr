/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/mbox/mbox_ti_omap_c7x.h>
#include <zephyr/sys/poweroff.h>

#include <kernel_arch_func.h>
#include <soc.h>

#define IPC_NODE DT_CHOSEN(zephyr_ipc)

FUNC_NORETURN void z_c7x_soc_system_halt(unsigned int reason)
{
	const struct mbox_dt_spec rx = MBOX_DT_SPEC_GET(IPC_NODE, rx);
	const struct mbox_dt_spec tx = MBOX_DT_SPEC_GET(IPC_NODE, tx);
	uint32_t ack = TI_K3_RP_MBOX_SHUTDOWN_ACK;
	const struct mbox_msg msg = {
		.data = &ack,
		.size = sizeof(ack),
	};
	uint32_t value = 0U;

	ARG_UNUSED(reason);

	/* the host resets this core only once it has acknowledged the shutdown request */
	while ((mbox_ti_omap_c7x_poll(rx.dev, rx.channel_id, &value) != 0) ||
	       (value != TI_K3_RP_MBOX_SHUTDOWN)) {
	}

	(void)mbox_send_dt(&tx, &msg);

	sys_poweroff();
}
