/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/exception.h>
#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <gen_offset.h>
#include <kernel_offsets.h>

GEN_OFFSET_STRUCT(arch_esf, rp);
GEN_OFFSET_STRUCT(arch_esf, nrp);
GEN_OFFSET_STRUCT(arch_esf, ntsr);
GEN_OFFSET_STRUCT(arch_esf, tsr);
GEN_OFFSET_STRUCT(arch_esf, ierr);
GEN_OFFSET_STRUCT(arch_esf, iesr);
GEN_OFFSET_STRUCT(arch_esf, iear);
GEN_OFFSET_STRUCT(arch_esf, ecsp);
GEN_OFFSET_STRUCT(arch_esf, sp);
GEN_OFFSET_STRUCT(arch_esf, vector);
GEN_OFFSET_STRUCT(arch_esf, a);

GEN_ABSOLUTE_SYM(__struct_arch_esf_SIZEOF, sizeof(struct arch_esf));

GEN_ABS_SYM_END
