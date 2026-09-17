/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 *
 *  More optimal vector memcpy/memset to override minimal libc's weak byte loops.
 */
#include <string.h>
#include <stdint.h>
#include <c7x.h>

void *memcpy(void *restrict dst, const void *restrict src, size_t n)
{
	uint8_t *d = dst;
	const uint8_t *s = src;

	while (n >= 64U) {
		*(__uchar64 *)d = *(const __uchar64 *)s;
		d += 64;
		s += 64;
		n -= 64U;
	}
	while (n >= 8U) {
		*(uint64_t *)d = *(const uint64_t *)s;
		d += 8;
		s += 8;
		n -= 8U;
	}
	while (n--) {
		*d++ = *s++;
	}
	return dst;
}

void *memset(void *dst, int c, size_t n)
{
	uint8_t *d = dst;
	uint64_t w = 0x0101010101010101ULL * (uint8_t)c;

	while (n >= 32U) {
		((uint64_t *)d)[0] = w;
		((uint64_t *)d)[1] = w;
		((uint64_t *)d)[2] = w;
		((uint64_t *)d)[3] = w;
		d += 32;
		n -= 32U;
	}
	while (n >= 8U) {
		*(uint64_t *)d = w;
		d += 8;
		n -= 8U;
	}
	while (n--) {
		*d++ = (uint8_t)c;
	}
	return dst;
}
