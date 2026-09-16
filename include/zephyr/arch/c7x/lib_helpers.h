/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief C7x control register accessors
 */

#ifndef ZEPHYR_INCLUDE_ARCH_C7X_LIB_HELPERS_H_
#define ZEPHYR_INCLUDE_ARCH_C7X_LIB_HELPERS_H_

#include <zephyr/toolchain.h>
#include <zephyr/types.h>

#include <c7x.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/* Event flag, set, clear and enable registers carry one bit per event line. */
static ALWAYS_INLINE uint64_t read_efr(void)
{
	return (uint64_t)__get_indexed(__EFR, 0);
}

static ALWAYS_INLINE void write_efclr(uint64_t mask)
{
	__set_indexed(__EFCLR, 0, mask);
}

static ALWAYS_INLINE uint64_t read_eer(void)
{
	return (uint64_t)__EER;
}

static ALWAYS_INLINE void write_eeset(uint64_t mask)
{
	__set_indexed(__EESET, 0, mask);
}

static ALWAYS_INLINE void write_eeclr(uint64_t mask)
{
	__set_indexed(__EECLR, 0, mask);
}

/* EPRI is indexed by event line. */
static ALWAYS_INLINE void write_epri(unsigned int evt, uint64_t val)
{
	__set_indexed(__EPRI, evt, val);
}

static ALWAYS_INLINE uint64_t read_ahpee(void)
{
	return (uint64_t)__AHPEE;
}

static ALWAYS_INLINE uint64_t read_tsc(void)
{
	return (uint64_t)__TSC;
}

static ALWAYS_INLINE uint64_t read_tsr(void)
{
	return (uint64_t)__TSR;
}

static ALWAYS_INLINE uint64_t read_ecsp_s(void)
{
	return (uint64_t)__ECSP_S;
}

static ALWAYS_INLINE void write_ecsp_s(uint64_t val)
{
	__ECSP_S = val;
}

static ALWAYS_INLINE uint64_t read_tcsp(void)
{
	return (uint64_t)__TCSP;
}

static ALWAYS_INLINE void write_tcsp(uint64_t val)
{
	__TCSP = val;
}

static ALWAYS_INLINE void write_estp_s(uint64_t val)
{
	__ESTP_S = val;
}

static ALWAYS_INLINE uint64_t read_ierr(void)
{
	return (uint64_t)__IERR;
}

static ALWAYS_INLINE uint64_t read_iear(void)
{
	return (uint64_t)__IEAR;
}

static ALWAYS_INLINE uint64_t read_iesr(void)
{
	return (uint64_t)__IESR;
}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_C7X_LIB_HELPERS_H_ */
