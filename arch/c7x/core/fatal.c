/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/exception.h>
#include <zephyr/toolchain.h>
#include <zephyr/fatal.h>
#include <zephyr/fatal_types.h>

#include <zephyr/arch/c7x/arch.h>
#include <kernel_arch_func.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

BUILD_ASSERT(sizeof(struct arch_esf) == C7X_EXC_FRAME_SIZE,
	     "arch_esf must match the frame c7x_fault_entry stores");
BUILD_ASSERT(offsetof(struct arch_esf, rp) == C7X_ESF_RP);
BUILD_ASSERT(offsetof(struct arch_esf, nrp) == C7X_ESF_NRP);
BUILD_ASSERT(offsetof(struct arch_esf, ntsr) == C7X_ESF_NTSR);
BUILD_ASSERT(offsetof(struct arch_esf, tsr) == C7X_ESF_TSR);
BUILD_ASSERT(offsetof(struct arch_esf, ierr) == C7X_ESF_IERR);
BUILD_ASSERT(offsetof(struct arch_esf, iesr) == C7X_ESF_IESR);
BUILD_ASSERT(offsetof(struct arch_esf, iear) == C7X_ESF_IEAR);
BUILD_ASSERT(offsetof(struct arch_esf, ecsp) == C7X_ESF_ECSP);
BUILD_ASSERT(offsetof(struct arch_esf, sp) == C7X_ESF_SP);
BUILD_ASSERT(offsetof(struct arch_esf, vector) == C7X_ESF_VECTOR);
BUILD_ASSERT(offsetof(struct arch_esf, a) == C7X_ESF_A0);

/* c7x_fault_entry parks the registers it needs as scratch here, and a second
 * entry while active is a fault inside the fault path.
 */
struct c7x_fault_state {
	uint64_t active;
	uint64_t vector;
	uint64_t sp;
	uint64_t a2;
	uint64_t a3;
};

volatile struct c7x_fault_state c7x_fault_state __used __aligned(64)
	Z_GENERIC_SECTION(.bss:c7x_diag);

#ifdef CONFIG_C7X_DBG_TRACE
volatile uint32_t c7x_dbg_trace[3] __used __aligned(64) Z_GENERIC_SECTION(.bss:c7x_diag);
#endif

#ifdef CONFIG_EXCEPTION_DEBUG
/* IERR bit names, starting from bit 0. */
static const char *const c7x_ierr_bits[] = {
	"PFX", "IFX", "FPX", "EPX", "OPX", "RCX", "RAX", "PRX",
	"LBX", "MSX", "DFX", "SEX", "EXX", "ADX", "MMX",
};

static void c7x_dump_fault(unsigned int reason, const struct arch_esf *esf)
{
	EXCEPTION_DUMP("C7x FATAL: reason=%u vector=%llu", reason,
		       (unsigned long long)esf->vector);
	EXCEPTION_DUMP("  NRP=0x%llx NTSR=0x%llx RP=0x%llx SP=0x%llx",
		       (unsigned long long)esf->nrp, (unsigned long long)esf->ntsr,
		       (unsigned long long)esf->rp, (unsigned long long)esf->sp);
	EXCEPTION_DUMP("  IERR=0x%llx IESR=0x%llx IEAR=0x%llx ECSP=0x%llx",
		       (unsigned long long)esf->ierr, (unsigned long long)esf->iesr,
		       (unsigned long long)esf->iear, (unsigned long long)esf->ecsp);

	for (unsigned int i = 0; i < ARRAY_SIZE(c7x_ierr_bits); i++) {
		if ((esf->ierr & BIT(i)) != 0ULL) {
			EXCEPTION_DUMP("  IERR %s", c7x_ierr_bits[i]);
		}
	}
}
#endif

FUNC_NORETURN void c7x_fatal_error(unsigned int reason, const struct arch_esf *esf)
{
#ifdef CONFIG_EXCEPTION_DEBUG
	if (esf != NULL) {
		c7x_dump_fault(reason, esf);
	} else {
		EXCEPTION_DUMP("C7x FATAL: reason=%u (no ESF)", reason);
	}
#endif

	z_fatal_error(reason, esf);

	k_fatal_halt(reason);

	/* an exception has no return path on C7x. No exception-exit context switch */
	CODE_UNREACHABLE;
}

FUNC_NORETURN void c7x_fault_handler(struct arch_esf *esf)
{
	uint64_t ecsp = read_ecsp_s();
	uint64_t ncnt = (ecsp & C7X_ECSP_NEST_MASK) >> C7X_ECSP_NEST_SHIFT;
	const uint64_t *ctx = ncnt ? (const uint64_t *)(uintptr_t)(ecsp + ((ncnt - 1) *
								   C7X_CONTEXT_SAVE_SIZE))
				   : (const uint64_t *)(uintptr_t)read_tcsp();

	esf->nrp = ctx[0];
	esf->ntsr = ctx[1];
	esf->tsr = read_tsr();
	esf->ierr = read_ierr();
	esf->iesr = read_iesr();
	esf->iear = read_iear();
	esf->ecsp = ecsp;

	c7x_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}
