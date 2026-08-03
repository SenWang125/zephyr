/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/arch/c7x/lib_helpers.h>

void z_sys_poweroff(void)
{
	for (;;) {
		z_c7x_idle();
	}
}
