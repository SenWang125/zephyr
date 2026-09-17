; Copyright (c) 2026 Texas Instruments Incorporated
; SPDX-License-Identifier: Apache-2.0

        .global c7x_mmu_mair_set
        .sect   ".text:c7x_mmu_mair_set"
        .asmfunc

c7x_mmu_mair_set:
        SHLD    .L1     A4, 3, A4
        SHLD    .L1     A5, A4, A5
        MVK64   .L1     0xff, A6
        MVC     .S1     MAR, A7
        SHLD    .L1     A6, A4, A6
        ANDND   .L1     A6, A7, A4
        ORD     .L1     A4, A5, A4
        MVC     .S1     A4, MAR
        MVC     .S1     A4, MAR
||      RET     .B1

        .endasmfunc

        .global c7x_mmu_tcr_set
        .sect   ".text:c7x_mmu_tcr_set"
        .asmfunc

c7x_mmu_tcr_set:
        MVC     .S1     A4, TCR0      ;; Table Control Register 0
||      RET     .B1

        .endasmfunc

        .global c7x_mmu_tbr0_set
        .sect   ".text:c7x_mmu_tbr0_set"
        .asmfunc

c7x_mmu_tbr0_set:
        MVC     .S1     A4, TBR0      ;; Table Base Register 0
||      RET     .B1
        NOP     5

        .endasmfunc

        .global c7x_mmu_tlb_inv
        .sect   ".text:c7x_mmu_tlb_inv"
        .asmfunc

c7x_mmu_tlb_inv:
        ;
        ; Don't write to the stack (or memory in general) before the
        ; cache WBINV operations.  This is because the MMU is typically
        ; disabled when this function is called and memory writes bypass
        ; the cache, and the WBINV might then overwrite that write.
        ;
        MVKU32  .L1     1, A5
        MVC     .S1     A5, L1DWBINV
        MVC     .S1     A4, TLB_INV
||      RET     .B1

        .endasmfunc

        .global c7x_l1d_wbinv
        .sect   ".text:c7x_l1d_wbinv"
        .asmfunc

c7x_l1d_wbinv:
        MVC     .S1     A4, L1DWBINV
||      RET     .B1

        .endasmfunc

        .global c7x_l1dcfg_get
        .sect   ".text:c7x_l1dcfg_get"
        .asmfunc
c7x_l1dcfg_get:
        MVC     .S1     L1DCFG, A4
||      RET     .B1
        .endasmfunc

        .global c7x_l1dcfg_set
        .sect   ".text:c7x_l1dcfg_set"
        .asmfunc
c7x_l1dcfg_set:
        MVC     .S1     A4, L1DCFG
        NOP     8
        RET     .B1
        NOP     5
        .endasmfunc

        .end
