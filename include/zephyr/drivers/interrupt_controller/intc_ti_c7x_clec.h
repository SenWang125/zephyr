/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_C7X_CLEC_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_C7X_CLEC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Disable every CLEC input and release its secure claim, before the core leaves its reset mode. */
void c7x_clec_access_init(void);

/* Route the devicetree events of C7x event line @p irq, the first time it is enabled. */
void c7x_clec_irq_enable(unsigned int irq);

/* The C7x event line the devicetree routes SoC event @p soc_event to, or -ENOENT. */
int c7x_clec_irq_of_event(uint32_t soc_event);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_C7X_CLEC_H_ */
