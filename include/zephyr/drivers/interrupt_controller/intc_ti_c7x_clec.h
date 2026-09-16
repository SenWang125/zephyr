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

/* Unmask @p local_irq (C7x internal event line) at the CLEC. */
void c7x_clec_irq_enable(unsigned int local_irq);

/* Route SoC event @p soc_event to C7x event line @p c7x_evt. */
void c7x_clec_route_program(uint32_t soc_event, uint32_t c7x_evt,
			    unsigned int is_level, unsigned int priority);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_C7X_CLEC_H_ */
