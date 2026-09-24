/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Register layout (K3, not the earlier OMAP layout):
 *    0x020  TIMER_IRQ_EOI        -- IRQ end-of-interrupt
 *    0x024  TIMER_IRQ_STATUS_RAW: raw status (read only)
 *    0x028  TIMER_IRQ_STATUS     -- IRQ status (write 1 to clear)
 *    0x02C  TIMER_IRQ_INT_ENABLE: IRQ enable set
 *    0x030  TIMER_IRQ_INT_DISABLE-- IRQ enable clear
 *    0x038  TIMER_TCLR           -- control (bit0=ST start, bit1=AR autoreload)
 *    0x03C  TIMER_TCRR           -- counter current value
 *    0x040  TIMER_TLDR           -- reload value
 *    0x048  TIMER_TWPS           -- write posted status (wait before each write)
 *
 *  CLEC routing. Event 120+256+i to C7x local interrupt 8+i (i=2 for TIMER2).
 *
 *  Tick mechanism:
 *    Load TLDR so the counter overflows every (25 MHz / TICKS_PER_SEC) cycles.
 *    countVal = 0xFFFFFFFF - counts_per_tick - 1
 *    For 1000 Hz. counts_per_tick = 25000, countVal = 0xFFFF9E56.
 */

#define DT_DRV_COMPAT ti_am654_timer

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <ctrl_partitions.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/interrupt_controller/intc_ti_c7x_clec.h>

#define TIMER_BASE           ((unsigned long)DT_INST_REG_ADDR(0))

#define TIMER_IRQ_EOI           (TIMER_BASE + 0x020U)
#define TIMER_IRQ_STATUS_RAW    (TIMER_BASE + 0x024U)
#define TIMER_IRQ_STATUS        (TIMER_BASE + 0x028U)
#define TIMER_IRQ_INT_ENABLE    (TIMER_BASE + 0x02CU)
#define TIMER_IRQ_INT_DISABLE   (TIMER_BASE + 0x030U)
#define TIMER_TCLR              (TIMER_BASE + 0x038U)
#define TIMER_TCRR              (TIMER_BASE + 0x03CU)
#define TIMER_TLDR              (TIMER_BASE + 0x040U)
#define TIMER_TWPS              (TIMER_BASE + 0x048U)

#define TIMER_OVF_INT_BIT     BIT(1)

#define TCLR_ST               BIT(0)
#define TCLR_AR               BIT(1)

#define TWPS_TCLR_PEND        BIT(0)
#define TWPS_TCRR_PEND        BIT(1)
#define TWPS_TLDR_PEND        BIT(2)

/*
 *  The dmtimer counts down from 0xFFFFFFFF, so a period of N input clocks loads
 *  as 0xFFFFFFFF - N - 1. The input clock is WKUP_OSC0_CLK, 25 MHz.
 */

#define TIMER_CLOCK_HZ   ((uint32_t)CONFIG_TI_DMTIMER_C7X_CLOCK_HZ)

#define TIMER_COUNTS_PER_TICK \
	(TIMER_CLOCK_HZ / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/*
 *  Down-counter reload value for one tick.
 */
#define TIMER_RELOAD \
	(0xFFFFFFFFU - TIMER_COUNTS_PER_TICK - 1U)

#define TIMER_IRQ    ((unsigned int)DT_INST_IRQ(0, irq))

static inline void timer_writel(uintptr_t addr, uint32_t val)
{
	sys_write32(val, addr);
}

static inline uint32_t timer_readl(uintptr_t addr)
{
	return sys_read32(addr);
}

/* a posted write settles in a few timer-clock cycles */
#define TIMER_TWPS_TIMEOUT_US	1000U

static inline void timer_wait_twps(uint32_t pend_mask)
{
	(void)WAIT_FOR((timer_readl(TIMER_TWPS) & pend_mask) == 0U, TIMER_TWPS_TIMEOUT_US, (void)0);
}

static void ti_dmtimer_c7x_isr(const void *arg)
{
	ARG_UNUSED(arg);

	timer_writel(TIMER_IRQ_STATUS, TIMER_OVF_INT_BIT);

	/* The posted write has been seen not to stick, and the event is
	 * level-routed, so an uncleared status re-asserts.
	 */
	if ((timer_readl(TIMER_IRQ_STATUS) & TIMER_OVF_INT_BIT) != 0U) {
		timer_writel(TIMER_IRQ_STATUS, TIMER_OVF_INT_BIT);
	}

	sys_clock_announce(1);
}

uint32_t sys_clock_elapsed(void)
{
	return 0U;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return arch_k_cycle_get_32();
}

uint64_t sys_clock_cycle_get_64(void)
{
	return arch_k_cycle_get_64();
}

void sys_clock_set_timeout(uint32_t ticks, bool idle)
{
	ARG_UNUSED(ticks);
	ARG_UNUSED(idle);
}

/* Start the timer and poll TWPS to confirm each posted write landed. */
static int sys_clock_driver_init(void)
{
#if DT_INST_NODE_HAS_PROP(0, clksel)
	{
		const uintptr_t syscon = DT_REG_ADDR(DT_INST_PHANDLE(0, clksel));
#if DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clksel), ti_unlock_offsets)
		/* A locked control-module partition drops the write silently. */
		static const uint32_t kick0[] = DT_PROP(DT_INST_PHANDLE(0, clksel), ti_unlock_offsets);

		for (size_t i = 0; i < ARRAY_SIZE(kick0); i++) {
			sys_write32(K3_CTRL_MMR_KICK0_UNLOCK_VAL, syscon + kick0[i]);
			sys_write32(K3_CTRL_MMR_KICK1_UNLOCK_VAL,
				    syscon + kick0[i] + 4U);
		}
#endif
		sys_write32(DT_INST_PHA(0, clksel, value),
			    syscon + DT_INST_PHA(0, clksel, offset));
#if DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clksel), ti_unlock_offsets)
		/* TI re-locks the partition after the write (SOC_controlModuleLockMMR). */
		for (size_t i = 0; i < ARRAY_SIZE(kick0); i++) {
			sys_write32(K3_CTRL_MMR_KICK_LOCK_VAL, syscon + kick0[i]);
			sys_write32(K3_CTRL_MMR_KICK_LOCK_VAL,
				    syscon + kick0[i] + 4U);
		}
#endif
	}
#endif

	timer_wait_twps(TWPS_TCLR_PEND);
	timer_writel(TIMER_TCLR, 0U);
	timer_wait_twps(TWPS_TCLR_PEND);

	timer_writel(TIMER_IRQ_INT_DISABLE, TIMER_OVF_INT_BIT);

	timer_writel(TIMER_IRQ_STATUS, TIMER_OVF_INT_BIT);

	timer_wait_twps(TWPS_TCRR_PEND);
	timer_writel(TIMER_TCRR, TIMER_RELOAD);
	timer_wait_twps(TWPS_TCRR_PEND);

	timer_wait_twps(TWPS_TLDR_PEND);
	timer_writel(TIMER_TLDR, TIMER_RELOAD);
	timer_wait_twps(TWPS_TLDR_PEND);

	timer_wait_twps(TWPS_TCLR_PEND);
	timer_writel(TIMER_TCLR, TCLR_AR | TCLR_ST);

	IRQ_CONNECT(DT_INST_IRQ(0, irq), DT_INST_IRQ(0, priority), ti_dmtimer_c7x_isr, NULL, 0);
	timer_writel(TIMER_IRQ_INT_ENABLE, TIMER_OVF_INT_BIT);
	c7x_clec_irq_enable(TIMER_IRQ);
	irq_enable(TIMER_IRQ);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
