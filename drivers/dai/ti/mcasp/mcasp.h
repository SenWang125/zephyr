/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DAI_TI_MCASP_H_
#define ZEPHYR_DRIVERS_DAI_TI_MCASP_H_

#include <zephyr/drivers/dai.h>
#include <zephyr/drivers/clock_control/tisci_clock_control.h>
#include <stdint.h>
#include <stdbool.h>

#define DAVINCI_MCASP_PWREMUMGT_REG	0x04
#define DAVINCI_MCASP_PFUNC_REG		0x10
#define DAVINCI_MCASP_PDIR_REG		0x14
#define DAVINCI_MCASP_PDIN_REG		0x1C
#define DAVINCI_MCASP_GBLCTL_REG	0x44
#define DAVINCI_MCASP_AMUTE_REG		0x48
#define DAVINCI_MCASP_LBCTL_REG		0x4C
#define DAVINCI_MCASP_TXDITCTL_REG	0x50
#define DAVINCI_MCASP_GBLCTLR_REG	0x60
#define DAVINCI_MCASP_RXMASK_REG	0x64
#define DAVINCI_MCASP_RXFMT_REG		0x68
#define DAVINCI_MCASP_RXFMCTL_REG	0x6C
#define DAVINCI_MCASP_ACLKRCTL_REG	0x70
#define DAVINCI_MCASP_AHCLKRCTL_REG	0x74
#define DAVINCI_MCASP_RXTDM_REG		0x78
#define DAVINCI_MCASP_RXSTAT_REG	0x80
#define DAVINCI_MCASP_RXCLKCHK_REG	0x88
#define DAVINCI_MCASP_EVTCTLR_REG	0x7C
#define DAVINCI_MCASP_REVTCTL_REG	0x8C
#define DAVINCI_MCASP_GBLCTLX_REG	0xA0
#define DAVINCI_MCASP_TXMASK_REG	0xA4
#define DAVINCI_MCASP_TXFMT_REG		0xA8
#define DAVINCI_MCASP_TXFMCTL_REG	0xAC
#define DAVINCI_MCASP_ACLKXCTL_REG	0xB0
#define DAVINCI_MCASP_AHCLKXCTL_REG	0xB4
#define DAVINCI_MCASP_TXTDM_REG		0xB8
#define DAVINCI_MCASP_TXSTAT_REG	0xC0
#define DAVINCI_MCASP_TXCLKCHK_REG	0xC8
#define DAVINCI_MCASP_EVTCTLX_REG	0xBC
#define DAVINCI_MCASP_XEVTCTL_REG	0xCC
#define DAVINCI_MCASP_XRSRCTL_REG(n)	(0x180 + ((n) << 2))
#define DAVINCI_MCASP_WFIFOCTL_REG	0x1000
#define DAVINCI_MCASP_WFIFOSTS_REG	0x1004
#define DAVINCI_MCASP_RFIFOCTL_REG	0x1008
#define DAVINCI_MCASP_RFIFOSTS_REG	0x100C

#define MCASP_FREE	BIT(0)
#define TXSSZ(val)	((val) << 4)
#define TXORD		BIT(15)
#define FSXDLY(val)	((val) << 16)
#define RXSSZ(val)	((val) << 4)
#define RXORD		BIT(15)
#define RXROT(val)	(val)
#define FSRDLY(val)	((val) << 16)
#define AFSXE		BIT(1)
#define FSXPOL		BIT(0)
#define FSXDUR		BIT(4)
#define FSXMOD(val)	((val) << 7)
#define AFSRE		BIT(1)
#define FSRPOL		BIT(0)
#define FSRDUR		BIT(4)
#define FSRMOD(val)	((val) << 7)
#define ACLKXDIV(val)	(val)
#define ACLKXE		BIT(5)
#define TX_ASYNC	BIT(6)
#define ACLKXPOL	BIT(7)
#define ACLKRPOL	BIT(7)
#define AHCLKXDIV(val)	(val)
#define AHCLKXE		BIT(15)
#define ACLKRDIV(val)	(val)
#define ACLKRE		BIT(5)
#define AHCLKRDIV(val)	(val)
#define AHCLKRE		BIT(15)
#define RXCLKRST	BIT(0)
#define RXHCLKRST	BIT(1)
#define RXSERCLR	BIT(2)
#define RXSMRST		BIT(3)
#define RXFSRST		BIT(4)
#define TXCLKRST	BIT(8)
#define TXHCLKRST	BIT(9)
#define TXSERCLR	BIT(10)
#define TXSMRST		BIT(11)
#define TXFSRST		BIT(12)
#define MODE(val)	(val)
/*
 * XRSRCTL[3:2]: what the pin drives during a slot this serializer is not
 * transmitting.
 */
#define DISMOD_3STATE	(0x0)
#define DISMOD_LOW	(0x2 << 2)
#define DISMOD_VAL(x)	((x) << 2)
#define PIN_BIT_AXR(n)	(n)
#define PIN_BIT_AMUTE	25
#define PIN_BIT_ACLKX	26
#define PIN_BIT_AHCLKX	27
#define PIN_BIT_AFSX	28
#define PIN_BIT_ACLKR	29
#define PIN_BIT_AHCLKR	30
#define PIN_BIT_AFSR	31
#define FIFO_ENABLE	BIT(16)
#define NUMEVT(x)	(((x) & 0xFF) << 8)
#define NUMDMA(x)	((x) & 0xFF)
#define CLKCHK_MAX(val)	((val) << 16)
#define DATDMA_DIS	BIT(0)
#define XSTAT_XUNDRN	BIT(0)		/* cslr_mcasp XSTAT_XUNDRN_MASK; bit 8 is XERR */
#define RSTAT_ROVRN	BIT(0)
#define XSTAT_XRERR	BIT(8)		/* composite error, both directions */
#define XSTAT_XRDATA	BIT(5)		/* transmit data ready */
#define MCASP_STAT_CLR	0xFFFFU
#define MCASP_XSTAT_ARM	0x1FFU

#define MCASP_ACLK_MAX 3		/* assigned-clocks entries the driver programs */

struct dai_ti_mcasp_cfg {
	uintptr_t base;
	uintptr_t dat_base;
	uint32_t  tx_dma_thread;
	uint32_t  rx_dma_thread;
	const struct device   *clk_dev;
	struct tisci_clock_config clk;
	uint32_t  clk_parent;
	uint32_t  clk_rate;
	uint8_t   n_aclk;
	uint32_t  aclk_id[MCASP_ACLK_MAX], aclk_parent[MCASP_ACLK_MAX], aclk_rate[MCASP_ACLK_MAX];
	uint8_t   tdm_slots;
	uint8_t   tdm_slots_rx;
	uint8_t   tx_num_evt;
	uint8_t   rx_num_evt;
	uint8_t   ser_dir[16];
	uint8_t   n_ser;
	uint8_t   dismod;
	bool      async_mode;
	uint16_t  self_ord;		/* DT dependency ordinal, matched against dai-link cpu */
	uint8_t   tx_irq, tx_irq_prio, rx_irq, rx_irq_prio;
	bool      has_tx_irq, has_rx_irq;
};

/* One simple-audio-card dai-link. */
struct mcasp_link {
	uint16_t cpu_ord;
	uint8_t  fmt;			/* MCASP_FMT_* */
	bool     bclk_inv, fs_inv;
	bool     cpu_bclk_master, cpu_fs_master;
	bool     playback_only, capture_only;
	uint8_t  slot_num, slot_width;	/* dai-tdm-slot-num / -width on the cpu end, 0 = unset */
	/* clocks / system-clock-frequency on the cpu end, 0 = none */
	uint32_t sysclk_hz;
	bool     sysclk_out;		/* system-clock-direction-out */
};

#define MCASP_FMT_DSP_A   0
#define MCASP_FMT_DSP_B   1
#define MCASP_FMT_I2S     2
#define MCASP_FMT_LEFT_J  3
#define MCASP_FMT_RIGHT_J 4

/* Wire geometry from the IPC4 gateway blob; 0 = topology supplied none. */
struct dai_ti_mcasp_wire {
	uint32_t tdm_slots;
	uint32_t tdm_slot_width;
};

struct dai_ti_mcasp_data {
	/* set_config() carries no direction but config_get() takes one, so a device
	 * whose directions differ needs a copy per direction. Latched at PRE_START.
	 */
	struct dai_config cfg_dir[2];
	bool cfg_dir_valid[2];
	struct dai_config cfg;
	struct dai_ti_mcasp_wire wire_dir[2];
	struct dai_ti_mcasp_wire wire;
	struct dai_properties props[2];
	uint32_t aux_hz;
	uint32_t aclk_hz[MCASP_ACLK_MAX];	/* granted rate per assigned-clocks entry */
	uint32_t tx_active_sers;
	uint32_t rx_active_sers;
	uint32_t tx_numevt;
	uint32_t rx_numevt;
	uint32_t tx_frame_div;
	bool tx_active;
	bool rx_active;
	struct mcasp_link tx_link;	/* resolved from the sound node at probe */
	struct mcasp_link rx_link;
	bool clk_acquired;
};

/* NUMEVT the TX FIFO was programmed with for this stream. mcasp_numevt()
 * shrinks the DT ceiling until it divides the period, and the DMA must follow it.
 */
uint32_t dai_ti_mcasp_tx_numevt(void);

#endif /* ZEPHYR_DRIVERS_DAI_TI_MCASP_H_ */
