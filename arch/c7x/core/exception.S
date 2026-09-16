; Copyright (c) 2026 Texas Instruments Incorporated
; SPDX-License-Identifier: Apache-2.0
;
; ======== exception.S ========
;

        .cdecls C,NOLIST,"c7x_frame.h","zephyr/arch/c7x/exception.h"

        .sect   ".data:c7x_isr_task_sp"
        .align  8
c7x_isr_task_sp:
        .word   0, 0
        .word   0, 0

;
;  ======== c7x_switch_dispatch ========
;
        .sect   ".text:c7x_switch_dispatch"
        .global c7x_switch_dispatch
        .global c7x_isr_task_sp
        .ref    c7x_isr_handler
        .align  8
        .asmfunc
c7x_switch_dispatch:
        ADDKPC  .D1     $PCR_OFFSET(c7x_isr_task_sp), A6
        LDD     .D1     *A6(8), A2
        LDD     .D1     *A6(0), A1
        ADDD    .D2     A2, -32, A5
        NOP     1
 [!A1]  STD     .D1     D15, *A6(0)
 [!A1]  MV      .D1     A5, D15                 ; set SP to isrStack
 [A1]   ADDD    .D2     D15, -32, D15
        STD     .D1     A8, *D15(16)            ; save SOE reg a8
||      STD     .D2X    A1, *D15(24)            ; save old taskSP on stack
||      MVC     .S1     RP, A8
        CALL    .B1     c7x_isr_handler
        LDD     .D1     *D15(24), A1            ; get old taskSP
        MVC     .S1     A8, RP
||      LDD     .D2     *D15(16), A8            ; restore SOE reg a8
        ADDKPC  .D1     $PCR_OFFSET(c7x_isr_task_sp), A6
||      ADDD    .D2     D15, 32, D15
 [!A1]  LDD     .D1     *A6(0), A3
 [!A1]  STD     .D1     A1, *A6(0)
 [!A1]  MV      .D1     A3, D15                 ; set SP to taskSP
        RET     .B1
        NOP     2
        .endasmfunc

        .global c7x_irq_lock
        .sect ".text:c7x_irq_lock"
        .clink
c7x_irq_lock:
        devt.s1 a4                              ; returns old TSR.GEE >> 25
||      ret.b1

        .global c7x_irq_unlock
        .sect ".text:c7x_irq_unlock"
        .clink
c7x_irq_unlock:
        revt.s1 a4
||      ret.b1

        .global c7x_set_cop
        .sect   ".text:c7x_set_cop"
        .clink
        .asmfunc

c7x_set_cop:
        SETCOP  .S1     A4
||      RET     .B1

        .endasmfunc

        .if $isdefed("CONFIG_C7X_DBG_TRACE")
        .ref    c7x_dbg_trace
DBG_TRACE_NME           .set    c7x_dbg_trace + 0x00
DBG_TRACE_RST           .set    c7x_dbg_trace + 0x08
        .endif

SP      .set    d15

; the event wrapper stashes A0/A1 in ECSP[0x300..1]; the fault entries take the next pair
ECSP_FAULT_A0   .set    0x302
ECSP_FAULT_A1   .set    0x303

||ISR_SCALAR_SZ||     .set    C7X_ISR_SCALAR_SIZE

||ISR_TASK_SP_OFF||   .set    0
||ISR_RP_OFF||        .set    8
||ISR_A2_OFF||        .set    16
||ISR_A3_OFF||        .set    24
||ISR_A4_OFF||        .set    32
||ISR_A5_OFF||        .set    40
||ISR_A6_OFF||        .set    48
||ISR_A7_OFF||        .set    56
||ISR_A8_OFF||        .set    64
||ISR_A9_OFF||        .set    72
||ISR_A10_OFF||       .set    80
||ISR_A11_OFF||       .set    88
||ISR_A12_OFF||       .set    96
||ISR_A13_OFF||       .set    104
||ISR_A14_OFF||       .set    112
||ISR_RXMR_OFF||      .set    120
||ISR_FPCR_OFF||      .set    232
||ISR_FSR_OFF||       .set    240
||ISR_TSR_OFF||       .set    248
||ISR_D0_OFF||        .set    272
||ISR_D1_OFF||        .set    280
||ISR_D2_OFF||        .set    288
||ISR_D3_OFF||        .set    296
||ISR_D4_OFF||        .set    304
||ISR_D5_OFF||        .set    312
||ISR_D6_OFF||        .set    320
||ISR_D7_OFF||        .set    328
||ISR_D8_OFF||        .set    336
||ISR_D9_OFF||        .set    344
||ISR_D10_OFF||       .set    352
||ISR_D11_OFF||       .set    360
||ISR_D12_OFF||       .set    368
||ISR_D13_OFF||       .set    376
||ISR_D14_OFF||       .set    384
||ISR_A15_OFF||       .set    456
||ISR_NRP_OFF||       .set    464
||ISR_NTSR_OFF||      .set    472
||ISR_A0HW_OFF||      .set    480
||ISR_A1HW_OFF||      .set    488
||ISR_AL0_OFF||       .set    496
||ISR_AL1_OFF||       .set    504
||ISR_AL2_OFF||       .set    512
||ISR_AL3_OFF||       .set    520
||ISR_AL4_OFF||       .set    528
||ISR_AL5_OFF||       .set    536
||ISR_AL6_OFF||       .set    544
||ISR_AL7_OFF||       .set    552
||ISR_AL8_OFF||       .set    560
||ISR_AL9_OFF||       .set    568
||ISR_AL10_OFF||      .set    576
||ISR_AL11_OFF||      .set    584
||ISR_AL12_OFF||      .set    592
||ISR_AL13_OFF||      .set    600
||ISR_AL14_OFF||      .set    608
||ISR_AL15_OFF||      .set    616
||ISR_AM0_OFF||       .set    624
||ISR_AM1_OFF||       .set    632
||ISR_AM2_OFF||       .set    640
||ISR_AM3_OFF||       .set    648
||ISR_AM4_OFF||       .set    656
||ISR_AM5_OFF||       .set    664
||ISR_AM6_OFF||       .set    672
||ISR_AM7_OFF||       .set    680
||ISR_AM8_OFF||       .set    688
||ISR_AM9_OFF||       .set    696
||ISR_AM10_OFF||      .set    704
||ISR_AM11_OFF||      .set    712
||ISR_AM12_OFF||      .set    720
||ISR_AM13_OFF||      .set    728
||ISR_AM14_OFF||      .set    736
||ISR_AM15_OFF||      .set    744
||ISR_GFPGFR_OFF||    .set    752
||ISR_GPLY_OFF||      .set    760

||ISR_VEC_P0_OFF||        .set    3904
||ISR_VEC_P1_OFF||        .set    3912
||ISR_VEC_P2_OFF||        .set    3920
||ISR_VEC_P3_OFF||        .set    3928
||ISR_VEC_P4_OFF||        .set    3936
||ISR_VEC_P5_OFF||        .set    3944
||ISR_VEC_P6_OFF||        .set    3952
||ISR_VEC_P7_OFF||        .set    3960
||ISR_VEC_TOTAL||         .set    C7X_ISR_VECTOR_SIZE

||ISR_VEC_SCALAR_SP_OFF|| .set    0

; 512 bit (64 byte) offsets
||ISR_VEC_VB0_OFF||       .set    64
||ISR_VEC_VB1_OFF||       .set    128
||ISR_VEC_VB2_OFF||       .set    192
||ISR_VEC_VB3_OFF||       .set    256
||ISR_VEC_VB4_OFF||       .set    320
||ISR_VEC_VB5_OFF||       .set    384
||ISR_VEC_VB6_OFF||       .set    448
||ISR_VEC_VB7_OFF||       .set    512
||ISR_VEC_VB8_OFF||       .set    576
||ISR_VEC_VB9_OFF||       .set    640
||ISR_VEC_VB10_OFF||      .set    704
||ISR_VEC_VB11_OFF||      .set    768
||ISR_VEC_VB12_OFF||      .set    832
||ISR_VEC_VB13_OFF||      .set    896
||ISR_VEC_VB14_OFF||      .set    960
||ISR_VEC_VB15_OFF||      .set    1024

||ISR_VEC_VBL0_OFF||      .set    1088
||ISR_VEC_VBL1_OFF||      .set    1152
||ISR_VEC_VBL2_OFF||      .set    1216
||ISR_VEC_VBL3_OFF||      .set    1280
||ISR_VEC_VBL4_OFF||      .set    1344
||ISR_VEC_VBL5_OFF||      .set    1408
||ISR_VEC_VBL6_OFF||      .set    1472
||ISR_VEC_VBL7_OFF||      .set    1536

||ISR_VEC_VBM0_OFF||      .set    1600
||ISR_VEC_VBM1_OFF||      .set    1664
||ISR_VEC_VBM2_OFF||      .set    1728
||ISR_VEC_VBM3_OFF||      .set    1792
||ISR_VEC_VBM4_OFF||      .set    1856
||ISR_VEC_VBM5_OFF||      .set    1920
||ISR_VEC_VBM6_OFF||      .set    1984
||ISR_VEC_VBM7_OFF||      .set    2048

||ISR_VEC_CUCR0_OFF||     .set    2112
||ISR_VEC_CUCR1_OFF||     .set    2176
||ISR_VEC_CUCR2_OFF||     .set    2240
||ISR_VEC_CUCR3_OFF||     .set    2304
||ISR_VEC_CUCR4_OFF||     .set    2368
||ISR_VEC_CUCR5_OFF||     .set    2432
||ISR_VEC_CUCR6_OFF||     .set    2496
||ISR_VEC_CUCR7_OFF||     .set    2560

; 4 SA Registers - these are stream as well - 3 vectors 512 bits long
||ISR_VEC_SA_SAVE_SZ||    .set    192
||ISR_VEC_SA0_OFF||       .set    2624
||ISR_VEC_SA1_OFF||       .set    2816
||ISR_VEC_SA2_OFF||       .set    3008
||ISR_VEC_SA3_OFF||       .set    3200

; 2 SE Registers - these are stream as well - 4 vectors 512 bits long
||ISR_VEC_SE_SAVE_SZ||    .set    256
||ISR_VEC_SE0_OFF||       .set    3392
||ISR_VEC_SE1_OFF||       .set    3648

        .ref    c7x_isr_exit_resched

        .sect   ".vecs"
        .global _vector_start
_vector_start:

        .global _z_vecs_reset
_z_vecs_reset:
        B       .B1     $PCR_OFFSET(c7x_rst_entry)

        .space  (0x200 - 4)

        .global _z_vecs_exc
_z_vecs_exc:
        B       .B1     $PCR_OFFSET(c7x_exc_entry)

        .space  (0x200 - 4)

        .global _z_vecs_pf
_z_vecs_pf:
        B       .B1     $PCR_OFFSET(c7x_pf_entry)

        .space  (0x200 - 4)

        .global _z_vecs_nme
_z_vecs_nme:
        B       .B1     $PCR_OFFSET(c7x_nme_entry)

        .space  (0x200 - 4)

        .global _z_vecs_evt
_z_vecs_evt:
        B       .B1     $PCR_OFFSET(c7x_evt_entry)
        NOP     5
        .space  (0x200 - 8)

        .sect   ".text:c7x_evt_entry"
        .global c7x_evt_entry
        .global _isr_wrapper
        .asmfunc

c7x_evt_entry:
_isr_wrapper:
        std     .D1     A0, *ECSP[0x300]
||      std     .D2X    A1, *ECSP[0x301]
        MVC     .S1     RP, A0
||      ADDD    .D1     SP, -||ISR_SCALAR_SZ||, SP ; alloc stack frame
||      ldd     .D2     *ECSP[1], A1
        NOP     1
        NOP     1
        STD     .D1     A0,  *SP(||ISR_RP_OFF||)
||      STD     .D2X    A3, *SP(||ISR_A3_OFF||)
        ldd     .D1     *ECSP[0], A0
||      MVC     .S1     TSR, A3
||      STD     .D2X    A2, *SP(||ISR_A2_OFF||)
        STD     .D1     A1, *SP(||ISR_NTSR_OFF||)
||      STD     .D2X    A5, *SP(||ISR_A5_OFF||)
||      MVC     .S1     FPCR, A2
        ldd     .D1     *ECSP[0x301], A1
||      MV      .M1     AM0, A5
||      STD     .D2X    A14, *SP(||ISR_A14_OFF||)
        STD     .D1     A3, *SP(||ISR_TSR_OFF||)
||      STD     .D2X    A6, *SP(||ISR_A6_OFF||)
        MV      .M1     AM7, A3
||      STD     .D1     A7, *SP(||ISR_A7_OFF||)
||      STD     .D2X    A11, *SP(||ISR_A11_OFF||)
        STD     .D1     A0, *SP(||ISR_NRP_OFF||)
||      STD     .D2X    A12, *SP(||ISR_A12_OFF||)
||      MV      .M1     AM1, A6
        ldd     .D1     *ECSP[0x300], A0
||      STD     .D2X    A13, *SP(||ISR_A13_OFF||)
||      MV      .M1     AM2, A7
        STD     .D1     A1, *SP(||ISR_A1HW_OFF||)
||      STD     .D2X    A3, *SP(||ISR_AM7_OFF||)
||      MV      .M1     AM3, A11
        MVC     .S1     FSR, A1
||      MV      .M1     AM15, A3
||      STD     .D1     A2, *SP(||ISR_FPCR_OFF||)
||      STD     .D2X    A5, *SP(||ISR_AM0_OFF||)
        ADDD    .D1     SP, ||ISR_SCALAR_SZ||, A14
||      MV      .M1     AM4, A12
||      STD     .D2X    A6, *SP(||ISR_AM1_OFF||)
        MV      .M1     AM5, A13
||      STD     .D1     A7, *SP(||ISR_AM2_OFF||)
||      STD     .D2X    A11, *SP(||ISR_AM3_OFF||)
        STD     .D1     A0, *SP(||ISR_A0HW_OFF||)
||      STD     .D2X    A1,  *SP(||ISR_FSR_OFF||)
||      MV      .M1     AM6, A2
        MV      .L1     AL0, A0
||      MV      .S1     AL1, A1
||      STD     .D1     A3, *SP(||ISR_AM15_OFF||)
||      MV      .M1     AM8, A5
||      STD     .D2X    A14, *SP(||ISR_TASK_SP_OFF||)
        MV      .D1     SP, A3
||      STD     .D2X    A12, *SP(||ISR_AM4_OFF||)
||      MV      .M1     AM9, A6
        LDD     .D1     *SP(||ISR_A14_OFF||), A14
||      STD     .D2X    A13, *SP(||ISR_AM5_OFF||)
||      MV      .M1     AM10, A7
        STD     .D1     A0, *SP(||ISR_AL0_OFF||)
||      STD     .D2X    A1, *SP(||ISR_AL1_OFF||)
||      MV      .M1     AM11, A11
        MV      .L1     AL2, A0
||      MV      .S1     AL3, A1
||      STD     .D1     A2, *SP(||ISR_AM6_OFF||)
||      STD     .D2X    A5, *SP(||ISR_AM8_OFF||)
||      MV      .M1     AM12, A12
        MV      .M1     AM13, A13
||      MVC     .S1     GFPGFR, A5
||      STD     .D1     A4, *SP(||ISR_A4_OFF||)
||      STD     .D2X    A8, *SP(||ISR_A8_OFF||)
        MV      .M1     AM14, A2
||      STD     .D1     A9, *SP(||ISR_A9_OFF||)
||      STD     .D2X    A10, *SP(||ISR_A10_OFF||)
||      MVC     .S1     AHPEE, A4
        STD     .D1     A0, *SP(||ISR_AL2_OFF||)
||      STD     .D2X    A1, *SP(||ISR_AL3_OFF||)
        MV      .L1     AL4, A0
||      MV      .S1     AL5, A1
||      STD     .D1     A15, *SP(||ISR_A15_OFF||)
||      STD     .D2X    A6, *SP(||ISR_AM9_OFF||)
        STD     .D1     D0, *SP(||ISR_D0_OFF||)
||      STD     .D2X    A7, *SP(||ISR_AM10_OFF||)
        STD     .D1     D1, *SP(||ISR_D1_OFF||)
||      STD     .D2X    A11, *SP(||ISR_AM11_OFF||)
        STD     .D1     A0, *SP(||ISR_AL4_OFF||)
||      STD     .D2X    A1, *SP(||ISR_AL5_OFF||)
        MV      .L1     AL6, A0
||      MV      .S1     AL7, A1
||      STD     .D1     A12, *SP(||ISR_AM12_OFF||)
||      STD     .D2X    A13, *SP(||ISR_AM13_OFF||)
        STD     .D1     D2, *SP(||ISR_D2_OFF||)
||      STD     .D2X    A2, *SP(||ISR_AM14_OFF||)
        STD     .D1     D3, *SP(||ISR_D3_OFF||)
||      STD     .D2X    A5, *SP(||ISR_GFPGFR_OFF||)
        STD     .D1     A0, *SP(||ISR_AL6_OFF||)
||      STD     .D2X    A1, *SP(||ISR_AL7_OFF||)
        MV      .L1     AL8, A0
||      MV      .S1     AL9, A1
||      STD     .D1     D4, *SP(||ISR_D4_OFF||)
        STD     .D1     D5, *SP(||ISR_D5_OFF||)
||      STD     .D2X    A0, *SP(||ISR_AL8_OFF||)
        STD     .D1     D6, *SP(||ISR_D6_OFF||)
||      STD     .D2X    A1, *SP(||ISR_AL9_OFF||)
||      MV      .L1     AL10, A0
        MV      .S1     AL11, A1
||      STD     .D1     D7, *SP(||ISR_D7_OFF||)
||      STD     .D2X    A0, *SP(||ISR_AL10_OFF||)
        STD     .D1     D8, *SP(||ISR_D8_OFF||)
||      STD     .D2X    A1, *SP(||ISR_AL11_OFF||)
||      MV      .L1     AL12, A0
        STD     .D1     D9, *SP(||ISR_D9_OFF||)
||      MV      .S1     AL13, A1
||      STD     .D2X    A0, *SP(||ISR_AL12_OFF||)
        STD     .D1     D10, *SP(||ISR_D10_OFF||)
||      STD     .D2X    A1, *SP(||ISR_AL13_OFF||)
||      MV      .L1     AL14, A0
        STD     .D1     D11, *SP(||ISR_D11_OFF||)
||      MV      .S1     AL15, A1
||      STD     .D2X    A0, *SP(||ISR_AL14_OFF||)
        STD     .D1     D12, *SP(||ISR_D12_OFF||)
||      STD     .D2X    A1, *SP(||ISR_AL15_OFF||)
        STD     .D1     D13, *SP(||ISR_D13_OFF||)
||      ANDD    .L1     A3, -64, A0
||      MVC     .S1     GPLY, A1
        STD     .D1     D14, *SP(||ISR_D14_OFF||)
        ADDD    .D1     A0, -||ISR_VEC_TOTAL||, A0
||      STD     .D2X    A1, *SP(||ISR_GPLY_OFF||)
||      MVC     .S1     RXMR_S, A2
        NOP     1
        STD     .D1     A2, *SP(||ISR_RXMR_OFF||)
        MV      .D1     A0, SP
        VST4D   .D2     VB0,  *SP(||ISR_VEC_VB0_OFF||)
||      STD     .D1     A3, *SP(||ISR_VEC_SCALAR_SP_OFF||)
        VMV     .S2     VBL0, VB0
||      VST4D   .D2     VB1,  *SP(||ISR_VEC_VB1_OFF||)
||      ADDD    .D1     SP, ||ISR_VEC_SA0_OFF||, A3 ;D15 is the SP register, per compiler ABI
        VST4D   .D2     VB2,  *SP(||ISR_VEC_VB2_OFF||)
||      VMV     .S2     VBL5, VB1
||      ADDD    .D1     SP, ||ISR_VEC_SA1_OFF||, A5 ;D15 is the SP register, per compiler ABI
        VST4D   .D2     VB3,  *SP(||ISR_VEC_VB3_OFF||)
||      VMV     .S2     VBL6, VB2
||      ADDD    .D1     SP, ||ISR_VEC_SA2_OFF||, A6 ;D15 is the SP register, per compiler ABI
        VST4D   .D2     VB0, *SP(||ISR_VEC_VBL0_OFF||)
||      VMV     .S2     VBL7, VB3
||      ADDD    .D1     SP, ||ISR_VEC_SA3_OFF||, A7 ;D15 is the SP register, per compiler ABI
        VMV     .S2     VBL1, VB0
||      VST4D   .D2     VB4,  *SP(||ISR_VEC_VB4_OFF||)
||      ADDD    .D1     SP, ||ISR_VEC_SE0_OFF||, A9 ;D15 is the SP register, per compiler ABI
        VST4D   .D2     VB5,  *SP(||ISR_VEC_VB5_OFF||)
||      VMV     .M2     VBM0, VB4
||      ADDD    .D1     SP, ||ISR_VEC_SE1_OFF||, A10 ;D15 is the SP register, per compiler ABI
        VST4D   .D2     VB6,  *SP(||ISR_VEC_VB6_OFF||)
||      VMV     .M2     VBM1, VB5
||      sasavest .D1     *A3(0), 0
        VST4D   .D2     VB0, *SP(||ISR_VEC_VBL1_OFF||)
||      VMV     .M2     VBM2, VB6
||      sasavest .D1     *A5(0), 1
        VMV     .S2     VBL2, VB0
||      VST4D   .D2     VB7,  *SP(||ISR_VEC_VB7_OFF||)
||      sasavest .D1     *A6(0), 2
        VST4D   .D2     VB1, *SP(||ISR_VEC_VBL5_OFF||)
||      VMV     .M2     VBM3, VB7
||      sasavest .D1     *A7(0), 3
        VST4D   .D2     VB2, *SP(||ISR_VEC_VBL6_OFF||)
||      VMV     .M2     VBM5, VB1
||      sesavest .D1     *A9(0), 0
        VST4D   .D2     VB0, *SP(||ISR_VEC_VBL2_OFF||)
||      VMV     .M2     VBM6, VB2
||      sesavest .D1     *A10(0), 1
        VST4D   .D2     VB3, *SP(||ISR_VEC_VBL7_OFF||)
||      VMV     .S2     VBL3, VB0
||      MV      .D1     SP, A0
        VMV     .M2     VBM7, VB3
||      VST4D   .D2     VB8,  *SP(||ISR_VEC_VB8_OFF||)
        VST4D   .D2     VB4, *SP(||ISR_VEC_VBM0_OFF||)
||      VMV     .M2     VBM4, VB8
        VST4D   .D2     VB0, *SP(||ISR_VEC_VBL3_OFF||)
||      MVC     .C2     CUCR0, VB4              ; 2 cycle latency for CUCR regs
        VST4D   .D2     VB5, *SP(||ISR_VEC_VBM1_OFF||)
||      VMV     .S2     VBL4, VB0
        VST4D   .D2     VB6, *SP(||ISR_VEC_VBM2_OFF||)
||      MVC     .C2     CUCR1, VB5
        VST4D   .D2     VB7, *SP(||ISR_VEC_VBM3_OFF||)
||      MVC     .C2     CUCR2, VB6
        VST4D   .D2     VB1, *SP(||ISR_VEC_VBM5_OFF||)
||      MVC     .C2     CUCR3, VB7
        VST4D   .D2     VB2, *SP(||ISR_VEC_VBM6_OFF||)
||      MVC     .C2     CUCR5, VB1              ; 2 cycle latency for CUCR regs
        VST4D   .D2     VB3, *SP(||ISR_VEC_VBM7_OFF||)
||      MVC     .C2     CUCR6, VB2              ; 2 cycle latency for CUCR regs
        MVC     .C2     CUCR7, VB3              ; 2 cycle latency for CUCR regs
||      VST4D   .D2     VB0, *SP(||ISR_VEC_VBL4_OFF||)
        VST4D   .D2     VB8, *SP(||ISR_VEC_VBM4_OFF||)
||      MVPB    .L2     P0, B0
        VST4D   .D2     VB4, *SP(||ISR_VEC_CUCR0_OFF||)
||      MVC     .C2     CUCR4, VB8              ; 2 cycle latency for CUCR regs
        VST4D   .D2     VB5, *SP(||ISR_VEC_CUCR1_OFF||)
||      MVPB    .L2     P4, B4
||      STD     .D1X    B0, *SP(||ISR_VEC_P0_OFF||)
        VST4D   .D2     VB6, *SP(||ISR_VEC_CUCR2_OFF||)
||      MVPB    .L2     P5, B5
||      VLD4D   .D1     *A0(||ISR_VEC_VB0_OFF||), VB0
        VST4D   .D2     VB7, *SP(||ISR_VEC_CUCR3_OFF||)
||      MVPB    .L2     P6, B6
||      STD     .D1X    B4, *SP(||ISR_VEC_P4_OFF||)
        VST4D   .D2     VB1, *SP(||ISR_VEC_CUCR5_OFF||)
||      MVPB    .L2     P7, B7
||      STD     .D1X    B5, *SP(||ISR_VEC_P5_OFF||)
        VST4D   .D2     VB2, *SP(||ISR_VEC_CUCR6_OFF||)
||      MVPB    .L2     P1, B1
||      STD     .D1X    B6, *SP(||ISR_VEC_P6_OFF||)
        VST4D   .D2     VB3, *SP(||ISR_VEC_CUCR7_OFF||)
||      MVPB    .L2     P2, B2
||      STD     .D1X    B7, *SP(||ISR_VEC_P7_OFF||)
        MVPB    .L2     P3, B3
||      VST4D   .D2     VB9,  *SP(||ISR_VEC_VB9_OFF||)
||      STD     .D1X    B1, *SP(||ISR_VEC_P1_OFF||)
        VST4D   .D2     VB10, *SP(||ISR_VEC_VB10_OFF||)
||      STD     .D1X    B2, *SP(||ISR_VEC_P2_OFF||)
        VST4D   .D2     VB11, *SP(||ISR_VEC_VB11_OFF||)
||      STD     .D1X    B3, *SP(||ISR_VEC_P3_OFF||)
        VST4D   .D2     VB12, *SP(||ISR_VEC_VB12_OFF||)
        VST4D   .D2     VB13, *SP(||ISR_VEC_VB13_OFF||)
        VST4D   .D2     VB14, *SP(||ISR_VEC_VB14_OFF||)
        VST4D   .D2     VB15, *SP(||ISR_VEC_VB15_OFF||)
        VST4D   .D2     VB8, *SP(||ISR_VEC_CUCR4_OFF||)
        ADDD    .D1     SP, -32, SP
        NOP     1
        NOP     1
        CALL    .B1     c7x_switch_dispatch
$C$RL_isr:
        CALL    .B1     c7x_isr_exit_resched
$C$RL_isr2:
        ADDD    .D1     SP, 32, SP

ISR_RESTORE_EXIT:
        MV      .D1     SP, A0
||      LDD     .D2     *SP(||ISR_VEC_P0_OFF||), B0
        VLD4D   .D1     *A0(||ISR_VEC_CUCR7_OFF||), VB1
||      LDD     .D2     *A0(||ISR_VEC_SCALAR_SP_OFF||), A2
        VLD4D   .D1     *A0(||ISR_VEC_CUCR6_OFF||), VB2
||      seclose .D2     0
        VLD4D   .D1     *A0(||ISR_VEC_CUCR5_OFF||), VB3
||      seclose .D2     1
||      MV      .L2     B0, P0
        VLD4D   .D1     *A0(||ISR_VEC_CUCR4_OFF||), VB4
        VLD4D   .D1     *A0(||ISR_VEC_CUCR3_OFF||), VB5
        VLD4D   .D1     *A0(||ISR_VEC_CUCR2_OFF||), VB6
||      MVC     .C2     VB1, CUCR7
        VLD4D   .D1     *A0(||ISR_VEC_CUCR1_OFF||), VB7
||      MVC     .C2     VB2, CUCR6
        VLD4D   .D1     *A0(||ISR_VEC_CUCR0_OFF||), VB8
||      MVC     .C2     VB3, CUCR5
||      LDD     .D2     *SP(||ISR_VEC_P1_OFF||), B1
        VLD4D   .D1     *A0(||ISR_VEC_VBM7_OFF||), VB9
||      MVC     .C2     VB4, CUCR4
||      LDD     .D2     *SP(||ISR_VEC_P2_OFF||), B2
        MVC     .C2     VB5, CUCR3
||      LDD     .D1     *SP(||ISR_VEC_P3_OFF||), B3
        MVC     .C2     VB6, CUCR2
||      LDD     .D1     *SP(||ISR_VEC_P4_OFF||), B4
||      MV      .L2     B1, P1
        MVC     .C2     VB7, CUCR1
||      LDD     .D1     *SP(||ISR_VEC_P5_OFF||), B5
||      MV      .L2     B2, P2
        LDD     .D1     *SP(||ISR_VEC_P6_OFF||), B6
||      MVC     .C2     VB8, CUCR0
||      MV      .L2     B3, P3
        LDD     .D1     *SP(||ISR_VEC_P7_OFF||), B7
||      VMV     .M2     VB9, VBM7
||      MV      .L2     B4, P4
        MV      .L2     B5, P5
||      VLD4D   .D1     *A0(||ISR_VEC_VBM6_OFF||), VB10
        MV      .L2     B6, P6
||      VLD4D   .D1     *A0(||ISR_VEC_VBM5_OFF||), VB11
        MV      .L2     B7, P7
||      VLD4D   .D1     *A0(||ISR_VEC_VBM4_OFF||), VB12
        VLD4D   .D1     *A0(||ISR_VEC_VBM3_OFF||), VB13
        VLD4D   .D1     *A0(||ISR_VEC_VBM2_OFF||), VB14
        VLD4D   .D1     *A0(||ISR_VEC_VBM1_OFF||), VB15
||      VMV     .M2     VB10, VBM6
        VLD4D   .D1     *A0(||ISR_VEC_VBM0_OFF||), VB1
||      VMV     .M2     VB11, VBM5
        VLD4D   .D1     *A0(||ISR_VEC_VBL7_OFF||), VB2
||      VMV     .M2     VB12, VBM4
        VLD4D   .D1     *A0(||ISR_VEC_VBL6_OFF||), VB3
||      VMV     .M2     VB13, VBM3
        VLD4D   .D1     *A0(||ISR_VEC_VBL5_OFF||), VB4
||      VMV     .M2     VB14, VBM2
        VLD4D   .D1     *A0(||ISR_VEC_VBL4_OFF||), VB5
||      VMV     .M2     VB15, VBM1
        VLD4D   .D1     *A0(||ISR_VEC_VBL3_OFF||), VB6
||      VMV     .M2     VB1, VBM0
        VLD4D   .D1     *A0(||ISR_VEC_VBL2_OFF||), VB7
||      VMV     .M2     VB2, VBL7
        VLD4D   .D1     *A0(||ISR_VEC_VBL1_OFF||), VB8
||      VMV     .M2     VB3, VBL6
        VLD4D   .D1     *A0(||ISR_VEC_VBL0_OFF||), VB9
||      VMV     .M2     VB4, VBL5
        ADDD    .D1     A0, ||ISR_VEC_SE1_OFF||, A3
||      VMV     .M2     VB5, VBL4
        ADDD    .D1     A0, ||ISR_VEC_SE0_OFF||, A5
||      VMV     .M2     VB6, VBL3
        VMV     .M2     VB7, VBL2
||      MV      .D1     A2, SP
        VMV     .M2     VB8, VBL1
||      serstrld .D1     *A3(0), 1
||      LDD     .D2     *SP(||ISR_GFPGFR_OFF||), A1
        VMV     .M2     VB9, VBL0
||      VLD4D   .D1     *A0(||ISR_VEC_VB15_OFF||), VB15
||      LDD     .D2     *SP(||ISR_AM15_OFF||), A2
        VLD4D   .D1     *A0(||ISR_VEC_VB14_OFF||), VB14
||      LDD     .D2     *SP(||ISR_AM13_OFF||), A4
        VLD4D   .D1     *A0(||ISR_VEC_VB13_OFF||), VB13
||      MVC     .S1     A1, GFPGFR
||      LDD     .D2     *SP(||ISR_D0_OFF||), D0
        VLD4D   .D1     *A0(||ISR_VEC_VB12_OFF||), VB12
||      MV      .M1     A2, AM15
||      LDD     .D2     *SP(||ISR_D1_OFF||), D1
        VLD4D   .D1     *A0(||ISR_VEC_VB11_OFF||), VB11
||      LDD     .D2     *SP(||ISR_AM10_OFF||), A1
||      MV      .M1     A4, AM13
        VLD4D   .D1     *A0(||ISR_VEC_VB10_OFF||), VB10
||      LDD     .D2     *SP(||ISR_AM9_OFF||), A2
        VLD4D   .D1     *A0(||ISR_VEC_VB9_OFF||),  VB9
||      LDD     .D2     *SP(||ISR_AM7_OFF||), A4
        VLD4D   .D1     *A0(||ISR_VEC_VB8_OFF||),  VB8
||      MV      .M1     A1, AM10
||      LDD     .D2     *SP(||ISR_D2_OFF||), D2
        VLD4D   .D1     *A0(||ISR_VEC_VB7_OFF||),  VB7
||      LDD     .D2     *SP(||ISR_AM4_OFF||), A1
||      MV      .M1     A2, AM9
        VLD4D   .D1     *A0(||ISR_VEC_VB6_OFF||),  VB6
||      LDD     .D2     *SP(||ISR_AM3_OFF||), A2
||      MV      .M1     A4, AM7
        VLD4D   .D1     *A0(||ISR_VEC_VB5_OFF||),  VB5
||      LDD     .D2     *SP(||ISR_AM1_OFF||), A4
        VLD4D   .D1     *A0(||ISR_VEC_VB4_OFF||),  VB4
||      MV      .M1     A1, AM4
||      LDD     .D2     *SP(||ISR_D3_OFF||), D3
        VLD4D   .D1     *A0(||ISR_VEC_VB3_OFF||),  VB3
||      LDD     .D2     *SP(||ISR_AL14_OFF||), A1
||      MV      .M1     A2, AM3
        VLD4D   .D1     *A0(||ISR_VEC_VB2_OFF||),  VB2
||      LDD     .D2     *SP(||ISR_AL13_OFF||), A2
||      MV      .M1     A4, AM1
        VLD4D   .D1     *A0(||ISR_VEC_VB1_OFF||),  VB1
||      LDD     .D2     *SP(||ISR_AL11_OFF||), A4
        VLD4D   .D1     *A0(||ISR_VEC_VB0_OFF||), VB0
||      MV      .L1     A1, AL14
||      LDD     .D2     *SP(||ISR_D4_OFF||), D4
        serstrld .D1     *A5(0), 0
||      LDD     .D2     *SP(||ISR_AL8_OFF||), A1
||      MV      .L1     A2, AL13
        ADDD    .D1     A0, ||ISR_VEC_SA1_OFF||, A6
||      saclose .C2     0
||      LDD     .D2     *SP(||ISR_AL7_OFF||), A2
||      MV      .L1     A4, AL11
        ADDD    .D1     A0, ||ISR_VEC_SA3_OFF||, A3
||      saclose .C2     1
||      LDD     .D2     *SP(||ISR_AL5_OFF||), A4
        ADDD    .D1     A0, ||ISR_VEC_SA2_OFF||, A5
||      saclose .C2     2
||      MV      .L1     A1, AL8
||      LDD     .D2     *SP(||ISR_D5_OFF||), D5
        ADDD    .D1     A0, ||ISR_VEC_SA0_OFF||, A7
||      saclose .C2     3
||      LDD     .D2     *SP(||ISR_AL2_OFF||), A1
||      MV      .L1     A2, AL7
        sarstrld .D1     *A3(0), 3
||      LDD     .D2     *SP(||ISR_AL1_OFF||), A2
||      MV      .L1     A4, AL5
        LDD     .D1     *SP(||ISR_AM14_OFF||), A3
||      LDD     .D2     *SP(||ISR_D6_OFF||), D6
        LDD     .D1     *SP(||ISR_GPLY_OFF||), A0
||      MV      .L1     A1, AL2
||      LDD     .D2     *SP(||ISR_D7_OFF||), D7
        sarstrld .D1     *A5(0), 2
||      LDD     .D2     *SP(||ISR_FSR_OFF||),  A1
||      MV      .L1     A2, AL1
        MV      .M1     A3, AM14
||      LDD     .D1     *SP(||ISR_AM12_OFF||), A5
||      LDD     .D2     *SP(||ISR_D8_OFF||), D8
        MVC     .S1     A0, GPLY
||      LDD     .D1     *SP(||ISR_AM8_OFF||), A3
||      LDD     .D2     *SP(||ISR_D9_OFF||), D9
        MVC     .S1     A1, FSR
||      sarstrld .D1     *A6(0), 1
||      LDD     .D2     *SP(||ISR_D10_OFF||), D10
        LDD     .D1     *SP(||ISR_AM11_OFF||), A0
||      MV      .M1     A5, AM12
||      LDD     .D2     *SP(||ISR_D11_OFF||), D11
        MV      .M1     A3, AM8
||      LDD     .D1     *SP(||ISR_AM6_OFF||), A5
||      LDD     .D2     *SP(||ISR_FPCR_OFF||), A1
        LDD     .D1     *SP(||ISR_AM2_OFF||), A3
||      LDD     .D2     *SP(||ISR_D12_OFF||), D12
        MV      .M1     A0, AM11
||      sarstrld .D1     *A7(0), 0
||      LDD     .D2     *SP(||ISR_D13_OFF||), D13
        LDD     .D1     *SP(||ISR_AM5_OFF||), A0
||      MV      .M1     A5, AM6
||      LDD     .D2     *SP(||ISR_D14_OFF||), D14
||      MVC     .S1     A1, FPCR
        MV      .M1     A3, AM2
||      LDD     .D1     *SP(||ISR_AM0_OFF||), A5
||      LDD     .D2     *SP(||ISR_A14_OFF||), A14
        LDD     .D1     *SP(||ISR_AL12_OFF||), A3
||      LDD     .D2     *SP(||ISR_A13_OFF||), A13
        MV      .M1     A0, AM5
||      LDD     .D1     *SP(||ISR_A15_OFF||), A15
||      LDD     .D2     *SP(||ISR_A12_OFF||), A12
        LDD     .D1     *SP(||ISR_AL15_OFF||), A0
||      MV      .M1     A5, AM0
||      LDD     .D2     *SP(||ISR_A11_OFF||), A11
        MV      .L1     A3, AL12
||      LDD     .D1     *SP(||ISR_AL10_OFF||), A5
||      LDD     .D2     *SP(||ISR_A10_OFF||), A10
        LDD     .D1     *SP(||ISR_AL6_OFF||), A3
||      LDD     .D2     *SP(||ISR_A9_OFF||), A9
        MV      .L1     A0, AL15
||      LDD     .D1     *SP(||ISR_A8_OFF||), A8
||      LDD     .D2     *SP(||ISR_A7_OFF||), A7
        LDD     .D1     *SP(||ISR_AL9_OFF||), A0
||      MV      .L1     A5, AL10
||      LDD     .D2     *SP(||ISR_A6_OFF||), A6
        MV      .L1     A3, AL6
||      LDD     .D1     *SP(||ISR_AL4_OFF||), A5
||      LDD     .D2     *SP(||ISR_A4_OFF||), A4
        LDD     .D1     *SP(||ISR_AL0_OFF||), A3
||      LDD     .D2     *SP(||ISR_A2_OFF||), A2
        MV      .L1     A0, AL9
        LDD     .D1     *SP(||ISR_AL3_OFF||), A0
||      MV      .L1     A5, AL4
        MV      .L1     A3, AL0
||      LDD     .D1     *SP(||ISR_A5_OFF||), A5
        LDD     .D1     *SP(||ISR_A3_OFF||), A3
        MV      .L1     A0, AL3
        LDD     .D1     *SP(||ISR_RP_OFF||), A0
||      LDD     .D2     *SP(||ISR_RXMR_OFF||), A1
        NOP     1
        NOP     1
        MVC     .S1     A0, RP
        ADDD    .D1     SP, ||ISR_SCALAR_SZ||, SP ; free stack frame
||      MVC     .S1     A1, RXMR_S

        ldd.d1    *ECSP[0], A0
||      ldd.d2    *ECSP[1], A1
        NOP     1
        rete.s1   A0, A1
||      ldd.d1    *ECSP[0x300], A0
||      ldd.d2    *ECSP[0x301], A1

        .endasmfunc

        .sect   ".text:c7x_nme_entry"
        .global c7x_nme_entry
        .asmfunc
c7x_nme_entry:
        .if $isdefed("CONFIG_C7X_DBG_TRACE")
        MVK64   .L1     DBG_TRACE_NME, A0
        NOP     4
        MVKU32  .L1     0x0000e7c3, A1
        NOP     4
        STW     .D1     A1, *A0(0)
        NOP     4
        MVC     .S1     RP, A1
        NOP     5
        STW     .D1     A1, *A0(4)
        NOP     4
        .endif
c7x_nme_spin:
        B       .B1     $PCR_OFFSET(c7x_nme_spin)
        NOP     5
        .endasmfunc

        .sect   ".text:c7x_rst_entry"
        .global c7x_rst_entry
        .asmfunc
c7x_rst_entry:
        .if $isdefed("CONFIG_C7X_DBG_TRACE")
        MVK64   .L1     DBG_TRACE_RST, A0
        NOP     4
        MVKU32  .L1     0x0000e7c1, A1
        NOP     4
        STW     .D1     A1, *A0(0)
        NOP     4
        .endif
c7x_rst_spin:
        B       .B1     $PCR_OFFSET(c7x_rst_spin)
        NOP     5
        .endasmfunc

        .sect   ".text:c7x_fault_entry"
        .global c7x_exc_entry
        .global c7x_pf_entry
        .ref    c7x_fault_state
        .ref    c7x_fault_handler
        .ref    c7x_isr_stack
        .asmfunc

C7X_FAULT_STACK_TOP     .set    c7x_isr_stack + CONFIG_ISR_STACK_SIZE - 16

C7X_FAULT_ACTIVE_OFF    .set    0
C7X_FAULT_VECTOR_OFF    .set    8
C7X_FAULT_SP_OFF        .set    16
C7X_FAULT_A2_OFF        .set    24
C7X_FAULT_A3_OFF        .set    32

c7x_exc_entry:
        std     .D1     A0, *ECSP[ECSP_FAULT_A0]
||      std     .D2X    A1, *ECSP[ECSP_FAULT_A1]
        MVK64   .L1     c7x_fault_state, A0
        NOP     4
        LDD     .D1     *A0(C7X_FAULT_ACTIVE_OFF), A1
        NOP     4
 [A1]   B       .B1     $PCR_OFFSET(c7x_fault_spin)
        NOP     5
        MVKU32  .L1     1, A1
        NOP     4
        STD     .D1     A1, *A0(C7X_FAULT_ACTIVE_OFF)
        MVKU32  .L1     C7X_FAULT_VECTOR_EXC, A1
        NOP     4
        STD     .D1     A1, *A0(C7X_FAULT_VECTOR_OFF)
        B       .B1     $PCR_OFFSET(c7x_fault_common)
        NOP     5

c7x_pf_entry:
        std     .D1     A0, *ECSP[ECSP_FAULT_A0]
||      std     .D2X    A1, *ECSP[ECSP_FAULT_A1]
        MVK64   .L1     c7x_fault_state, A0
        NOP     4
        LDD     .D1     *A0(C7X_FAULT_ACTIVE_OFF), A1
        NOP     4
 [A1]   B       .B1     $PCR_OFFSET(c7x_fault_spin)
        NOP     5
        MVKU32  .L1     1, A1
        NOP     4
        STD     .D1     A1, *A0(C7X_FAULT_ACTIVE_OFF)
        MVKU32  .L1     C7X_FAULT_VECTOR_PF, A1
        NOP     4
        STD     .D1     A1, *A0(C7X_FAULT_VECTOR_OFF)
        B       .B1     $PCR_OFFSET(c7x_fault_common)
        NOP     5

c7x_fault_common:
        STD     .D1     D15, *A0(C7X_FAULT_SP_OFF)
        STD     .D1     A2, *A0(C7X_FAULT_A2_OFF)
        STD     .D1     A3, *A0(C7X_FAULT_A3_OFF)
        MVK64   .L1     C7X_FAULT_STACK_TOP, A2
        NOP     4
        MV      .D1     A2, D15
        NOP     1
        ADDD    .D1     SP, -C7X_EXC_FRAME_SIZE, SP
        NOP     1
        STD     .D1     A4,  *SP(C7X_ESF_A0 + 0x20)
        STD     .D1     A5,  *SP(C7X_ESF_A0 + 0x28)
        STD     .D1     A6,  *SP(C7X_ESF_A0 + 0x30)
        STD     .D1     A7,  *SP(C7X_ESF_A0 + 0x38)
        STD     .D1     A8,  *SP(C7X_ESF_A0 + 0x40)
        STD     .D1     A9,  *SP(C7X_ESF_A0 + 0x48)
        STD     .D1     A10, *SP(C7X_ESF_A0 + 0x50)
        STD     .D1     A11, *SP(C7X_ESF_A0 + 0x58)
        STD     .D1     A12, *SP(C7X_ESF_A0 + 0x60)
        STD     .D1     A13, *SP(C7X_ESF_A0 + 0x68)
        STD     .D1     A14, *SP(C7X_ESF_A0 + 0x70)
        STD     .D1     A15, *SP(C7X_ESF_A0 + 0x78)
        ldd     .D1     *ECSP[ECSP_FAULT_A0], A2
        NOP     4
        STD     .D1     A2, *SP(C7X_ESF_A0 + 0x00)
        ldd     .D1     *ECSP[ECSP_FAULT_A1], A2
        NOP     4
        STD     .D1     A2, *SP(C7X_ESF_A0 + 0x08)
        LDD     .D1     *A0(C7X_FAULT_A2_OFF), A2
        NOP     4
        STD     .D1     A2, *SP(C7X_ESF_A0 + 0x10)
        LDD     .D1     *A0(C7X_FAULT_A3_OFF), A2
        NOP     4
        STD     .D1     A2, *SP(C7X_ESF_A0 + 0x18)
        LDD     .D1     *A0(C7X_FAULT_SP_OFF), A2
        NOP     4
        STD     .D1     A2, *SP(C7X_ESF_SP)
        LDD     .D1     *A0(C7X_FAULT_VECTOR_OFF), A2
        NOP     4
        STD     .D1     A2, *SP(C7X_ESF_VECTOR)
        MVC     .S1     RP, A2
        NOP     5
        STD     .D1     A2, *SP(C7X_ESF_RP)
        MV      .D1     SP, A4
        NOP     1
        ADDD    .D1     SP, -32, SP
        CALL    .B1     c7x_fault_handler
        NOP     5

c7x_fault_spin:
        B       .B1     $PCR_OFFSET(c7x_fault_spin)
        NOP     5

        .endasmfunc

        .end
