/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

void z_sys_poweroff(void)
{
	for (;;) {
		__asm__(" IDLE\n NOP 8\n");
	}

	CODE_UNREACHABLE;
}
