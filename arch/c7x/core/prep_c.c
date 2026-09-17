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

/* ---- boot stack and control-stack storage ---- */

#define C7X_SECTION(name) __used Z_GENERIC_SECTION(.bss:name)

/* The nest count has to address every level of the area allocated below. */
BUILD_ASSERT((C7X_ECSP_NEST_MASK >> C7X_ECSP_NEST_SHIFT) == C7X_ECSP_NEST_LEVELS - 1U);

char c7x_ecsp_area[C7X_ECSP_SIZE] C7X_SECTION(c7x_ecsp_area) __aligned(C7X_ECSP_SIZE);

char c7x_tcsp_area[C7X_CONTEXT_SAVE_SIZE] C7X_SECTION(c7x_tcsp_area)
	__aligned(C7X_CONTEXT_SAVE_SIZE);

char c7x_isr_stack[CONFIG_ISR_STACK_SIZE] Z_GENERIC_SECTION(.c7x_isr_stack_l2)
	__aligned(C7X_EABI_SP_ALIGN);
#define C7X_ISR_STACK_TOP	(c7x_isr_stack + sizeof(c7x_isr_stack))

/* volatile. The compiler must not recognise this loop and call memset. */
void arch_early_memset(void *dst, int c, size_t n)
{
	volatile uint64_t *w = dst;
	volatile char *b;
	uint64_t fill = 0x0101010101010101ULL * (uint8_t)c;

	while (n >= 8U) {
		*w++ = fill;
		n -= 8U;
	}
	b = (volatile char *)w;
	while (n-- != 0U) {
		*b++ = (char)c;
	}
}

FUNC_NORETURN void z_prep_c(void)
{
	soc_prep_hook();

	arch_bss_zero();
	arch_data_copy();

	c7x_l1d_wbinv(C7X_L1D_WBINV_ALL);

	c7x_mm_init();

#ifdef CONFIG_INIT_STACKS
	/* Populate stack before any interrupt events */
	arch_early_memset(c7x_isr_stack, 0xaa, CONFIG_ISR_STACK_SIZE);
#endif

	z_cstart();

	CODE_UNREACHABLE;
}

FUNC_NORETURN void c7x_boot_init(void)
{
	write_ecsp_s((uint64_t)(uintptr_t)c7x_ecsp_area);
	write_tcsp((uint64_t)(uintptr_t)c7x_tcsp_area);

	/* ISR SP for c7x_switch_dispatch's state block */
	c7x_isr_task_sp[1] = (uint64_t)(uintptr_t)C7X_ISR_STACK_TOP;
	write_estp_s((uint64_t)(uintptr_t)&_z_vecs_reset);

	/* Clear stale EFR and EER in case of warm boot */
	write_efclr(UINT64_MAX);
	write_eeclr(UINT64_MAX);

	c7x_set_cop(C7X_TSR_COP_ALL);

	z_prep_c();

	CODE_UNREACHABLE;
}
