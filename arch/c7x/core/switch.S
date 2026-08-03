; Copyright (c) 2026 Texas Instruments Incorporated
; SPDX-License-Identifier: Apache-2.0
;
; ======== switch.S ========
;
        .cdecls C,NOLIST,"zephyr/offsets.h"

||SL_A8||        .set  __struct_c7x_switch_frame_a8_OFFSET
||SL_A9||        .set  __struct_c7x_switch_frame_a9_OFFSET
||SL_A10||       .set  __struct_c7x_switch_frame_a10_OFFSET
||SL_A11||       .set  __struct_c7x_switch_frame_a11_OFFSET
||SL_A12||       .set  __struct_c7x_switch_frame_a12_OFFSET
||SL_A13||       .set  __struct_c7x_switch_frame_a13_OFFSET
||SL_A14||       .set  __struct_c7x_switch_frame_a14_OFFSET
||SL_A15||       .set  __struct_c7x_switch_frame_a15_OFFSET
||SL_B14||       .set  __struct_c7x_switch_frame_b14_OFFSET
||SL_B15||       .set  __struct_c7x_switch_frame_b15_OFFSET
||SL_TCSP||      .set  __struct_c7x_switch_frame_tcsp_OFFSET
||SL_TSR||       .set  __struct_c7x_switch_frame_tsr_OFFSET
||SL_GLUE_RP||   .set  __struct_c7x_switch_frame_glue_rp_OFFSET
||SL_RP||        .set  __struct_c7x_switch_frame_rp_OFFSET
||FRSZ||         .set  __struct_c7x_switch_frame_SIZEOF

        .ref    z_c7x_thread_entry_wrapper

;
;  ======== z_c7x_switch ========
;
        .sect   ".text:c7x_switch"
        .global z_c7x_switch
        .align  8
        .asmfunc

z_c7x_switch:
        MVC     .S1     RP, A6
||      ADDD    .D1     D15, -||FRSZ||, A7
        MVC     .S1     TCSP, A2
||      STD     .D1     A8, *A7(||SL_A8||)
||      STD     .D2X    A9, *A7(||SL_A9||)
        MVC     .S1     TSR, A3
||      STD     .D1     A10, *A7(||SL_A10||)
||      STD     .D2X    A11, *A7(||SL_A11||)
        STD     .D1     A12, *A7(||SL_A12||)
||      STD     .D2X    A13, *A7(||SL_A13||)
        STD     .D1     A14, *A7(||SL_A14||)
||      STD     .D2X    A15, *A7(||SL_A15||)
        STD     .D1X    B14, *A7(||SL_B14||)
||      STD     .D2     B15, *A7(||SL_B15||)
        STD     .D1     A2, *A7(||SL_TCSP||)
||      STD     .D2X    A3, *A7(||SL_TSR||)
        STD     .D1     A6, *A7(||SL_GLUE_RP||)
||      STD     .D2X    A6, *A7(||SL_RP||)
        STD     .D1     A7, *A5

        LDD     .D1     *A4(||SL_TCSP||), A3
||      LDD     .D2     *A4(||SL_TSR||), A2
        LDD     .D1     *A4(||SL_GLUE_RP||), A1
||      LDD     .D2     *A4(||SL_RP||), A0
        LDD     .D1     *A4(||SL_A8||), A8
||      LDD     .D2     *A4(||SL_A9||), A9
        LDD     .D1     *A4(||SL_A10||), A10
||      LDD     .D2     *A4(||SL_A11||), A11
        LDD     .D1     *A4(||SL_A12||), A12
||      LDD     .D2     *A4(||SL_A13||), A13
        LDD     .D1     *A4(||SL_A14||), A14
||      LDD     .D2     *A4(||SL_A15||), A15
||      MVC     .S1     A3, TCSP
        LDD     .D1     *A4(||SL_B14||), B14
||      LDD     .D2     *A4(||SL_B15||), B15
||      MVC     .S1     A0, RP
||      ANDW    .M1     A2, 8, A0
        ADDD    .D1     A4, ||FRSZ||, D15
|| [!A0] B      .B1     z_c7x_switch_thread_mode    ; branch to exit if in thread mode

        EHDL                     ; enable handler mode
        NOP     1
        BA      .S1     A1

z_c7x_switch_thread_mode:
        DHDL                     ; enable thread mode
        NOP     1
        BA      .S1     A1

        .endasmfunc

;
;  ======== z_c7x_thread_start ========
;
;  This function is executed immediately before the thread entry point
;  passed to k_thread_create().  This function loads the parameters to
;  z_thread_entry() into registers and necessary for the C7x register passing model.
;
        .sect   ".text:c7x_thread_start"
        .global z_c7x_thread_start
        .asmfunc

z_c7x_thread_start:
        MV      .D1     A8,  A4         ; arg 0
        MV      .D1     A9,  A5         ; arg 1
        MV      .D1     A10, A6
        MV      .D1     A11, A7
        CALL    .B1     z_c7x_thread_entry_wrapper

        .endasmfunc

        .end
