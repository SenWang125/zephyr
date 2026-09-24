/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/arch/common/xip.h>
#include <zephyr/arch/c7x/cache.h>
#include <zephyr/arch/c7x/mmu.h>
#include <zephyr/platform/hooks.h>
#include <kernel_internal.h>
#include <c7x.h>

/* The nest count has to address every level of the area allocated below. */
BUILD_ASSERT((C7X_ECSP_NEST_MASK >> C7X_ECSP_NEST_SHIFT) == C7X_ECSP_NEST_LEVELS - 1U);

char z_c7x_ecsp_area[C7X_ECSP_SIZE] __used Z_GENERIC_SECTION(.c7x_ecsp_area)
	__aligned(C7X_ECSP_SIZE);

char z_c7x_tcsp_area[C7X_CONTEXT_SAVE_SIZE] __used Z_GENERIC_SECTION(.c7x_tcsp_area)
	__aligned(C7X_CONTEXT_SAVE_SIZE);

char z_c7x_isr_stack[CONFIG_ISR_STACK_SIZE] __used __aligned(0x1000)
	Z_GENERIC_SECTION(.c7x_isr_stack);
#define C7X_ISR_STACK_TOP	(z_c7x_isr_stack + sizeof(z_c7x_isr_stack))

/* [0] saves the interrupted stack's SP and is 0 outside an ISR, [1] is the ISR stack top */
uint64_t z_c7x_isr_task_sp[2] __used __aligned(64) Z_GENERIC_SECTION(.c7x_isr_stack);

FUNC_NORETURN void z_prep_c(void)
{
	soc_prep_hook();

	z_c7x_mm_init();

	arch_bss_zero();
	arch_data_copy();

	z_c7x_isr_task_sp[0] = 0U;
	z_c7x_isr_task_sp[1] = (uint64_t)(uintptr_t)C7X_ISR_STACK_TOP;

	z_c7x_l1d_wbinv(C7X_L1D_WBINV_ALL);

#ifdef CONFIG_C7X_ILUT
	__ILTER = __ILUT_RW;
#endif

#ifdef CONFIG_INIT_STACKS
	/* Populate stack before any interrupt events */
	arch_early_memset(z_c7x_isr_stack, 0xaa, CONFIG_ISR_STACK_SIZE);
#endif

	z_cstart();

	CODE_UNREACHABLE;
}

static FUNC_NORETURN void z_c7x_boot_nonsecure(void);

FUNC_NORETURN void z_c7x_boot_init(void)
{
	z_c7x_write_ecsp_s((uint64_t)(uintptr_t)z_c7x_ecsp_area);
	z_c7x_write_tcsp((uint64_t)(uintptr_t)z_c7x_tcsp_area);
	z_c7x_write_estp_s((uint64_t)(uintptr_t)&_z_vecs_reset);

	/* Clear stale EFR and EER in case of warm boot */
	z_c7x_write_efclr(UINT64_MAX);
	z_c7x_write_eeclr(UINT64_MAX);

	z_c7x_exit_secure((z_c7x_read_tsr() & ~C7X_TSR_CXM_MASK) | C7X_CXM_S,
			   (uint64_t)(uintptr_t)z_c7x_boot_nonsecure);

	CODE_UNREACHABLE;
}

static FUNC_NORETURN void z_c7x_boot_nonsecure(void)
{
	z_c7x_write_estp_current((uint64_t)(uintptr_t)&_z_vecs_reset);

	z_c7x_set_cop(C7X_TSR_COP_TASK_MODE);

	z_prep_c();

	CODE_UNREACHABLE;
}
