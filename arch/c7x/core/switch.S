; Copyright (c) 2026 Texas Instruments Incorporated
; SPDX-License-Identifier: Apache-2.0
;
; ======== switch.S ========
;
        .cdecls C,NOLIST,"c7x_frame.h"

||SL_RESERVED0|| .set  8 * C7X_CFRAME_SLOT_RESERVED0
||SL_RESERVED1|| .set  8 * C7X_CFRAME_SLOT_RESERVED1
||SL_A8||        .set  8 * C7X_CFRAME_SLOT_A8
||SL_A9||        .set  8 * C7X_CFRAME_SLOT_A9
||SL_A10||       .set  8 * C7X_CFRAME_SLOT_A10
||SL_A11||       .set  8 * C7X_CFRAME_SLOT_A11
||SL_A12||       .set  8 * C7X_CFRAME_SLOT_A12
||SL_A13||       .set  8 * C7X_CFRAME_SLOT_A13
||SL_A14||       .set  8 * C7X_CFRAME_SLOT_A14
||SL_A15||       .set  8 * C7X_CFRAME_SLOT_A15
||SL_B14||       .set  8 * C7X_CFRAME_SLOT_B14
||SL_B15||       .set  8 * C7X_CFRAME_SLOT_B15
||SL_TCSP||      .set  8 * C7X_CFRAME_SLOT_TCSP
||SL_TSR||       .set  8 * C7X_CFRAME_SLOT_TSR
||SL_GLUE_RP||   .set  8 * C7X_CFRAME_SLOT_GLUE_RP
||SL_RP||        .set  8 * C7X_CFRAME_SLOT_RP
||FRSZ||         .set  8 * C7X_CFRAME_SLOTS

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

        ADDD    .D1     D15, -||FRSZ||, A7
        MVC     .S1     TCSP, A2
        MVC     .S1     TSR,  A3

        STD     .D1     A8, *A7(||SL_A8||)
        ||      STD     .D2X    A9, *A7(||SL_A9||)
        STD     .D1     A10, *A7(||SL_A10||)
        ||      STD     .D2X    A11, *A7(||SL_A11||)
        STD     .D1     A12, *A7(||SL_A12||)
        ||      STD     .D2X    A13, *A7(||SL_A13||)
        STD     .D1     A14, *A7(||SL_A14||)
        ||      STD     .D2X    A15, *A7(||SL_A15||)

        STD     .D1X    B14, *A7(||SL_B14||)
        ||      STD     .D2     B15, *A7(||SL_B15||)

        STD     .D1     A2, *A7(||SL_TCSP||)
        ||      STD     .D2X    A3, *A7(||SL_TSR||)

        STD     .D1     A6,  *A7(||SL_GLUE_RP||)
        STD     .D1     A6,  *A7(||SL_RP||)

        MVK64   .L1     0x0, A8
        STD     .D1     A8,  *A7(||SL_RESERVED0||)
        STD     .D1     A8,  *A7(||SL_RESERVED1||)

        STD     .D1     A7, *A5

        LDD     .D1     *A4(||SL_A8||),  A8
||      LDD     .D2     *A4(||SL_A9||),  A9
        LDD     .D1     *A4(||SL_A10||),  A10
||      LDD     .D2     *A4(||SL_A11||),  A11
        LDD     .D1     *A4(||SL_A12||),  A12
||      LDD     .D2     *A4(||SL_A13||),  A13
        LDD     .D1     *A4(||SL_A14||),  A14
||      LDD     .D2     *A4(||SL_A15||),  A15

        LDD     .D1     *A4(||SL_B14||), B14
||      LDD     .D2     *A4(||SL_B15||), B15

        LDD     .D1     *A4(||SL_TCSP||), A3
        LDD     .D1     *A4(||SL_TSR||), A2
        LDD     .D1     *A4(||SL_GLUE_RP||), A1
        LDD     .D1     *A4(||SL_RP||), A0

        MVC     .S1     A3, TCSP

        ADDD    .D1     A4, ||FRSZ||, D15

        MVC     .S1     A0, RP

        ANDW    .M1     A2, 8, A0
 [!A0]  B       .B1     z_c7x_switch_thread_mode    ; branch to exit if in thread mode

        EHDL                     ; enable handler mode
        BA      .S1     A1

z_c7x_switch_thread_mode:
        DHDL                     ; enable thread mode
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
