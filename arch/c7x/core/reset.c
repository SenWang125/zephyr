/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/c7x/cpu.h>
#include <kernel_internal.h>

/* Reset entry. Set SP as position independent, before handing to boot flow. */
#pragma CODE_SECTION(_c_int00_secure, ".text:_c_int00_secure")
FUNC_NORETURN void _c_int00_secure(void)
{
	__SP = (((uint64_t)(uintptr_t)z_interrupt_stacks + sizeof(z_interrupt_stacks[0])) -
		C7X_EABI_FREE_AREA) &
	       ~(uint64_t)(C7X_EABI_SP_ALIGN - 1U);

	c7x_boot_init();
}
