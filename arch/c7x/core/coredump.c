/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/coredump.h>

#define ARCH_HDR_VER			1

struct c7x_arch_block {
	struct {
		uint64_t nrp;
		uint64_t ntsr;
		uint64_t rp;
		uint64_t tsr;
		uint64_t ierr;
		uint64_t iesr;
		uint64_t iear;
		uint64_t ecsp;
		uint64_t sp;
		uint64_t vector;
		uint64_t a[16];
	} r;
} __packed;

static struct c7x_arch_block arch_blk;

void arch_coredump_info_dump(const struct arch_esf *esf)
{
	struct coredump_arch_hdr_t hdr = {
		.id = COREDUMP_ARCH_HDR_ID,
		.hdr_version = ARCH_HDR_VER,
		.num_bytes = sizeof(arch_blk),
	};

	if (esf == NULL) {
		return;
	}

	(void)memset(&arch_blk, 0, sizeof(arch_blk));

	arch_blk.r.nrp = esf->nrp;
	arch_blk.r.ntsr = esf->ntsr;
	arch_blk.r.rp = esf->rp;
	arch_blk.r.tsr = esf->tsr;
	arch_blk.r.ierr = esf->ierr;
	arch_blk.r.iesr = esf->iesr;
	arch_blk.r.iear = esf->iear;
	arch_blk.r.ecsp = esf->ecsp;
	arch_blk.r.sp = esf->sp;
	arch_blk.r.vector = esf->vector;

	for (unsigned int i = 0; i < ARRAY_SIZE(arch_blk.r.a); i++) {
		arch_blk.r.a[i] = esf->a[i];
	}

	coredump_buffer_output((uint8_t *)&hdr, sizeof(hdr));
	coredump_buffer_output((uint8_t *)&arch_blk, sizeof(arch_blk));
}

uint16_t arch_coredump_tgt_code_get(void)
{
	return COREDUMP_TGT_C7X;
}

#if defined(CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP)
uintptr_t arch_coredump_stack_ptr_get(const struct k_thread *thread)
{
	if (thread == NULL) {
		return 0;
	}

	/* Only the faulting thread's SP is in the dumped frame. Every other
	 * thread's is in its switch handle.
	 */
	if (thread == _current) {
		return (uintptr_t)arch_blk.r.sp;
	}

	return (uintptr_t)thread->switch_handle;
}
#endif /* CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP */
