/*
 *  Copyright (c) 2026 Texas Instruments Incorporated
 *  SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_C7X_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_C7X_INCLUDE_KERNEL_ARCH_FUNC_H_

#include <kernel_arch_data.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

/* the stack pointer, for the reset entry and the boot-stack paint */
register volatile uint64_t __SP;

static ALWAYS_INLINE void arch_kernel_init(void)
{
}

static inline bool arch_is_in_isr(void)
{
	return _kernel.cpus[0].nested != 0U;
}

extern FUNC_NORETURN void c7x_fatal_error(unsigned int reason,
					     const struct arch_esf *esf);

FUNC_NORETURN void c7x_boot_init(void);
FUNC_NORETURN void z_prep_c(void);

void c7x_switch(void *switch_to, void **switched_from);

static ALWAYS_INLINE void arch_switch(void *switch_to, void **switched_from)
{
	c7x_switch(switch_to, switched_from);
}

/* entered from assembly */
void z_c7x_thread_entry_wrapper(k_thread_entry_t entry, void *p1, void *p2, void *p3);
void c7x_isr_handler(unsigned int evt_id);
void c7x_isr_exit_resched(void);
FUNC_NORETURN void c7x_fault_handler(struct arch_esf *esf);

/* defined in assembly */
void _z_vecs_reset(void);
void c7x_thread_start(void);
extern uint64_t c7x_isr_task_sp[2];

extern char c7x_ecsp_area[];
extern char c7x_tcsp_area[];
extern char c7x_isr_stack[];

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_C7X_INCLUDE_KERNEL_ARCH_FUNC_H_ */
