/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DAI_TI_MCASP_H_
#define ZEPHYR_INCLUDE_DRIVERS_DAI_TI_MCASP_H_

#include <zephyr/device.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * Describes the serial wire. The codec's slot width is fixed by its register binary.
 */
struct dai_ti_mcasp_blob {
	uint32_t gw_attr;
	uint32_t tdm_slots;
	uint32_t tdm_slot_width;
};

/* Detect a latched XUNDRN and restart the transmitter. Called from the DMA
 * get_status path only when it is already starved. This SoC wires no McASP
 * interrupt to the C7x, so there is nothing to hook.
 */
int dai_ti_mcasp_tx_recover_underrun(void);

/* TX AFIFO event size as programmed for the current stream, 0 if bypassed. */
uint32_t dai_ti_mcasp_tx_numevt(void);

/* TX burst width in FIFO words for a stream of this channel count. The DAI
 * cannot answer from its own state before the stream configures it, and that
 * state is shared between directions, so the caller supplies the width.
 */
uint32_t dai_ti_mcasp_tx_burst(const struct device *dev, uint32_t channels);

/* RX burst width in FIFO words for a stream of this channel count. Same reason
 * as TX. The DAI cannot answer from its own state before a CAPTURE has
 * configured it, and a playback does not populate the RX side.
 */
uint32_t dai_ti_mcasp_rx_burst(const struct device *dev, uint32_t channels);

/* Called from the BCDMA driver's completion path. It owns the transfer, the
 * McASP owns the FIFO status. Public because a DMA driver must not reach into
 * a DAI driver's private header.
 */
void mcasp_poll_txstat(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DAI_TI_MCASP_H_ */
