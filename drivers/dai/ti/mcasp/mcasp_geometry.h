/*
 *  Copyright (c) 2026 Texas Instruments Incorporated
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Scalar geometry rules, separated from the device model so a host test can
 *  compile the shipped code rather than a copy of it.
 */

#ifndef ZEPHYR_DRIVERS_DAI_TI_MCASP_GEOMETRY_H_
#define ZEPHYR_DRIVERS_DAI_TI_MCASP_GEOMETRY_H_

#include <stdint.h>

/* How many serializers this channel count needs, 0 if the geometry cannot
 * carry it. A count the DAI refuses here but ALSA advertised opens, passes
 * hw_params and fails -EIO at trigger, which is what 3, 5 and 7 did on a
 * 4-serializer by 2-slot board.
 */
static inline uint32_t mcasp_sers_needed(uint32_t n_ser_avail, uint32_t slots_per_ser,
					 uint32_t channels)
{
	uint32_t need_ser;

	if (channels == 0U || slots_per_ser == 0U) {
		return 0U;
	}
	if (channels >= slots_per_ser && (channels % slots_per_ser) != 0U) {
		return 0U;
	}
	need_ser = (channels < slots_per_ser) ? 1U : channels / slots_per_ser;
	return (need_ser == 0U || need_ser > n_ser_avail) ? 0U : need_ser;
}

/* golden davinci-mcasp:1184-1200 -- shrink NUMEVT in whole-serializer steps
 * until it divides the period, then give the DMA that same value.
 */
static inline uint32_t mcasp_numevt(uint32_t ceiling, uint32_t active_sers,
				    uint32_t period_bytes)
{
	uint32_t period_words = period_bytes / 4U;
	uint32_t n;

	if (active_sers == 0U) {
		active_sers = 1U;
	}
	uint32_t step = active_sers;

	/* ceiling 0 = AFIFO bypassed (davinci-mcasp:1157); no burst, no divisibility
	 * requirement, and none of the NUMEVT words of latency it would hold
	 */
	if (ceiling == 0U) {
		return 0U;
	}
	/* the DMA burst is this value divided by 4, so keep it a whole number of
	 * words there too or the two sides round to different bursts
	 */
	while ((step % 4U) != 0U) {
		step += active_sers;
	}
	n = (ceiling / step) * step;
	if (n == 0U) {
		n = step;
	}
	/* period unknown at this trigger: keep the ceiling rather than guess */
	if (period_words == 0U) {
		return n;
	}
	while (n > step && (period_words % n) != 0U) {
		n -= step;
	}
	if ((period_words % n) != 0U) {
		n = step;
	}
	return n;
}

#endif /* ZEPHYR_DRIVERS_DAI_TI_MCASP_GEOMETRY_H_ */
