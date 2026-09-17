/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_am62dx_mcasp

#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/interrupt_controller/intc_ti_c7x_clec.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dai.h>
#include <zephyr/drivers/dai/ti_mcasp.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <errno.h>
#include <stdint.h>
#include "mcasp.h"
#include "mcasp_geometry.h"

static const struct device *const dmsc = DEVICE_DT_GET(DT_NODELABEL(dmsc));
#include <zephyr/drivers/firmware/tisci/tisci.h>
#include <zephyr/cache.h>

LOG_MODULE_REGISTER(dai_ti_mcasp, CONFIG_DAI_LOG_LEVEL);

#define MCASP_ASSERT_MODE(inst) \
BUILD_ASSERT(DT_INST_PROP(inst, op_mode) == 0, \
	     "only op-mode 0 (I2S/TDM) is supported");
DT_INST_FOREACH_STATUS_OKAY(MCASP_ASSERT_MODE)

static inline void mcasp_wr(uintptr_t base, uint32_t offset, uint32_t value)
{
	sys_write32(value, base + offset);
}

static inline uint32_t mcasp_rd(uintptr_t base, uint32_t offset)
{
	return sys_read32(base + offset);
}

#define mcasp_reg_write(off, val)  mcasp_wr(c->base, (off), (val))
#define mcasp_reg_read(off)        mcasp_rd(c->base, (off))

/* Spin bound for the start-time FIFO prime and XRDATA waits. A spin, not a timer. */
#define MCASP_TX_PRIME_SPINS 100000U

/* TISCI_MSG_VALUE_DEVICE_HW_STATE_ON. A read in state TRANS(2) hangs the bus */
#define MCASP_PM_HW_STATE_ON	   1U
#define MCASP_PM_SETTLE_SPINS	   256U
#define MCASP_DIAG_SETTLE_INIT	   30U
#define MCASP_DIAG_SETTLE_STOP	   31U

static inline void mcasp_dsb(void)
{
	barrier_dsync_fence_full();
}

/* simple-audio-card links, compile-time from the sound node(s). Matched to this
 * instance by the cpu sound-dai ordinal at probe
 */
#define MCASP_ACLK_CELL(inst, prop, i)						\
COND_CODE_1(DT_INST_PROP_HAS_IDX(inst, prop, i),			\
	    (DT_INST_PHA_BY_IDX(inst, prop, i, clkid)), (0))
#define MCASP_ACLK_RATE(inst, i)						\
COND_CODE_1(DT_INST_PROP_HAS_IDX(inst, assigned_clock_rates, i),	\
	    (DT_INST_PROP_BY_IDX(inst, assigned_clock_rates, i)), (0))

#define MCASP_LINK_ENTRY(link)							\
{									\
	.cpu_ord = DT_DEP_ORD(DT_PHANDLE(DT_CHILD(link, cpu), sound_dai)),	\
	.fmt = DT_ENUM_IDX(link, format),				\
	.bclk_inv = DT_PROP(link, bitclock_inversion),			\
	.fs_inv = DT_PROP(link, frame_inversion),			\
	.cpu_bclk_master = COND_CODE_1(DT_NODE_HAS_PROP(link, bitclock_master), \
		(DT_SAME_NODE(DT_PHANDLE(link, bitclock_master),		\
			      DT_PHANDLE(DT_CHILD(link, cpu), sound_dai))), (1)), \
	.cpu_fs_master = COND_CODE_1(DT_NODE_HAS_PROP(link, frame_master), \
		(DT_SAME_NODE(DT_PHANDLE(link, frame_master),		\
			      DT_PHANDLE(DT_CHILD(link, cpu), sound_dai))), (1)), \
	.playback_only = DT_PROP(link, playback_only),			\
	.capture_only = DT_PROP(link, capture_only),			\
	.slot_num = DT_PROP_OR(DT_CHILD(link, cpu), dai_tdm_slot_num, 0), \
	.slot_width = DT_PROP_OR(DT_CHILD(link, cpu), dai_tdm_slot_width, 0), \
	.sysclk_hz = COND_CODE_1(DT_NODE_HAS_PROP(DT_CHILD(link, cpu), clocks), \
		(DT_PROP(DT_CLOCKS_CTLR(DT_CHILD(link, cpu)), clock_frequency)), \
		(DT_PROP_OR(DT_CHILD(link, cpu), system_clock_frequency, 0))), \
	.sysclk_out = DT_PROP(DT_CHILD(link, cpu), system_clock_direction_out), \
},
#define MCASP_LINKS_OF_CARD(card) DT_FOREACH_CHILD(card, MCASP_LINK_ENTRY)

static const struct mcasp_link mcasp_links[] = {
#if DT_HAS_COMPAT_STATUS_OKAY(simple_audio_card)
	DT_FOREACH_STATUS_OKAY(simple_audio_card, MCASP_LINKS_OF_CARD)
#endif
};

/* Defaults when no link names this DAI: dsp_a, normal clocks, McASP provides both */
static const struct mcasp_link mcasp_link_default = {
	.fmt = MCASP_FMT_DSP_A, .cpu_bclk_master = true, .cpu_fs_master = true,
};

static void mcasp_links_resolve(const struct dai_ti_mcasp_cfg *c, struct dai_ti_mcasp_data *d)
{
	bool tx = false, rx = false;

	d->tx_link = mcasp_link_default;
	d->rx_link = mcasp_link_default;
	for (size_t i = 0; i < ARRAY_SIZE(mcasp_links); i++) {
		if (mcasp_links[i].cpu_ord != c->self_ord) {
			continue;
		}
		if (!mcasp_links[i].capture_only) {
			d->tx_link = mcasp_links[i];
			tx = true;
		}
		if (!mcasp_links[i].playback_only) {
			d->rx_link = mcasp_links[i];
			rx = true;
		}
	}
	if (!tx || !rx) {
		LOG_WRN("no dai-link for %s%s: davinci defaults", tx ? "" : "playback ",
			rx ? "" : "capture");
	}
}

/* Slot geometry, in precedence order. The dai-link end, the McASP node's
 * tdm-slots, then the stream's blob. d->wire is shared with the other direction.
 */
static uint32_t mcasp_slots_for(const struct dai_ti_mcasp_cfg *c,
				const struct dai_ti_mcasp_data *d, enum dai_dir dir)
{
	const struct mcasp_link *l = (dir == DAI_DIR_TX) ? &d->tx_link : &d->rx_link;
	uint32_t node = (dir == DAI_DIR_TX) ? c->tdm_slots :
	(c->tdm_slots_rx != 0U ? c->tdm_slots_rx : c->tdm_slots);

	if (l->slot_num != 0U) {
		return l->slot_num;
	}
	if (node != 0U) {
		return node;
	}
	if (d->wire_dir[dir].tdm_slots != 0U) {
		return d->wire_dir[dir].tdm_slots;
	}
	return 2U;
}

/* A cpu-end clock reaches set_sysclk with direction IN unless the link sets
 * system-clock-direction-out, which leaves the AHCLK pins as inputs.
 */
static uint32_t mcasp_ext_hclk(const struct dai_ti_mcasp_data *d, enum dai_dir dir)
{
	const struct mcasp_link *l = (dir == DAI_DIR_TX) ? &d->tx_link : &d->rx_link;

	return (l->sysclk_hz != 0U && !l->sysclk_out) ? l->sysclk_hz : 0U;
}

static uint32_t mcasp_slot_width_for(const struct dai_ti_mcasp_data *d, enum dai_dir dir,
				     uint32_t stream_bits)
{
	const struct mcasp_link *l = (dir == DAI_DIR_TX) ? &d->tx_link : &d->rx_link;

	if (l->slot_width != 0U) {
		return l->slot_width;
	}
	if (d->wire_dir[dir].tdm_slot_width != 0U) {
		return d->wire_dir[dir].tdm_slot_width;
	}
	return stream_bits;
}

/* The DAI format decides frame-sync width, data delay and the I2S polarity
 * flip. The link flags do the rest.
 */
static inline uint32_t mcasp_fmt_data_delay(uint8_t fmt)
{
	return (fmt == MCASP_FMT_DSP_A || fmt == MCASP_FMT_I2S) ? 1U : 0U;
}

static inline bool mcasp_fmt_word_fs(uint8_t fmt)
{
	return fmt == MCASP_FMT_I2S || fmt == MCASP_FMT_LEFT_J || fmt == MCASP_FMT_RIGHT_J;
}

static inline bool mcasp_fs_pol_falling(const struct mcasp_link *l)
{
	return (l->fmt == MCASP_FMT_I2S) ? !l->fs_inv : l->fs_inv;
}

volatile uint32_t g_mcasp_rx_overruns;
extern volatile uint32_t g_mcasp_tx_underruns;

/* XUNDRN is write-1-to-clear, so acking it in the ISR would hide it from
 * dai_ti_mcasp_tx_recover_underrun(), which resets the transmitter.
 */
volatile uint32_t g_mcasp_tx_underrun_pending;

/* Count, and ack only what was handled */
static void mcasp_tx_irq(const void *arg)
{
	const struct device *dev = arg;
	const struct dai_ti_mcasp_cfg *c = dev->config;
	uint32_t stat = mcasp_rd(c->base, DAVINCI_MCASP_TXSTAT_REG);
	uint32_t handled = 0U;

	if (stat & XSTAT_XUNDRN) {
		g_mcasp_tx_underruns++;
		g_mcasp_tx_underrun_pending = 1U;
		handled |= XSTAT_XUNDRN;
	}
	if (stat & XSTAT_XRERR) {
		handled |= XSTAT_XRERR;
	}
	mcasp_wr(c->base, DAVINCI_MCASP_TXSTAT_REG, handled);
}

static void mcasp_rx_irq(const void *arg)
{
	const struct device *dev = arg;
	const struct dai_ti_mcasp_cfg *c = dev->config;
	uint32_t stat = mcasp_rd(c->base, DAVINCI_MCASP_RXSTAT_REG);
	uint32_t handled = 0U;

	if (stat & RSTAT_ROVRN) {
		g_mcasp_rx_overruns++;
		handled |= RSTAT_ROVRN;
	}
	if (stat & XSTAT_XRERR) {
		handled |= XSTAT_XRERR;
	}
	mcasp_wr(c->base, DAVINCI_MCASP_RXSTAT_REG, handled);
}

static int mcasp_gblctl_set(const struct dai_ti_mcasp_cfg *c, uint32_t bit)
{
	/* Read-modify-write through the per-side alias. A combined-GBLCTL write-back can
	 * re-assert the other side's resets.
	 */
	uint32_t alias = (bit & 0xFFU) ? DAVINCI_MCASP_GBLCTLR_REG
				   : DAVINCI_MCASP_GBLCTLX_REG;
	uint32_t v = mcasp_rd(c->base, alias) | bit;
	int i;

	mcasp_wr(c->base, alias, v);
	for (i = 0; i < 30000; i++) {
		if ((mcasp_rd(c->base, DAVINCI_MCASP_GBLCTL_REG) & bit) == bit) {
			return 0;
		}
	}
	LOG_ERR("GBLCTL bit 0x%x did not latch", bit);
	return -EIO;
}

/*
 *  A register access while the module is mid power-transition hangs the bus with
 *  no exception. TISCI reports HW_STATE_TRANS there. Only hw_init and
 *  hw_stop gate on this. The other 6 register-touching functions do not, and
 *  that gap is UNPROVEN. check_mcasp_gate_claim.py holds the count.
 */
/*
 * TXSTAT is armed at start and then never read again while audio runs, so a
 * latched XUNDRN sits there unseen. Off unless g_mcasp_xundrn_poll is armed.
 */
/* Last point reached on the trigger/quiesce path. The DDR mapping is
 * write-back cached, so the store must be flushed or the A53 reads a stale
 * eviction. Volatile only binds the compiler, not the cache.
 */
volatile uint32_t g_mcasp_bc;
void c7x_bc_set(uint32_t n)
{
	g_mcasp_bc = n;
	sys_cache_data_flush_range((void *)&g_mcasp_bc, sizeof(g_mcasp_bc));
}
#define BC(n)	c7x_bc_set(n)

static const struct dai_ti_mcasp_cfg *g_mcasp_cfg_active;
volatile uint32_t g_mcasp_xundrn_events;
volatile uint32_t g_mcasp_txstat_last;
volatile uint32_t g_mcasp_xundrn_poll;

void mcasp_poll_txstat(void)
{
	const struct dai_ti_mcasp_cfg *c = g_mcasp_cfg_active;
	uint32_t st;

	if (!g_mcasp_xundrn_poll || c == NULL) {
		return;
	}
	st = mcasp_reg_read(DAVINCI_MCASP_TXSTAT_REG);
	g_mcasp_txstat_last = st;
	if (st & XSTAT_XUNDRN) {
		g_mcasp_xundrn_events++;
		g_mcasp_tx_underrun_pending = 1U;
		mcasp_reg_write(DAVINCI_MCASP_TXSTAT_REG, XSTAT_XUNDRN);
	}
}

static bool mcasp_module_is_on(const struct dai_ti_mcasp_cfg *c, unsigned int slot)
{
	extern void c7x_diag2(unsigned int idx, uint32_t val);
	uint32_t clcnt = 0U, resets = 0U;
	uint8_t  p_state = 0U, c_state = 0xFFU;
	uint32_t spins = 0U;
	int rc = 0;

	if (c == NULL) {
		return false;
	}
	/* A failed query is retried, not fatal. TISCI answers with an error while the
	 * module is still settling.
	 */
	while (spins < MCASP_PM_SETTLE_SPINS) {
		rc = tisci_get_device_state(dmsc, c->clk.dev_id, &clcnt, &resets,
					    &p_state, &c_state);
		if (rc == 0 && c_state == MCASP_PM_HW_STATE_ON) {
			break;
		}
		spins++;
	}
	c7x_diag2(slot, 0xF4000000U | ((spins & 0x3FFFU) << 10) |
		    (((uint32_t)c_state & 0xFFU) << 4) | ((uint32_t)rc & 0xFU));
	if (c_state != MCASP_PM_HW_STATE_ON) {
		LOG_ERR("McASP dev %u state %u after %u spins (last rc %d); not touching registers",
			c->clk.dev_id, c_state, spins, rc);
		return false;
	}
	return true;
}

/* Serializers this stream uses, by the rule mcasp_hw_init() applies to need_ser.
 * config_set() has stored the channel count by the time the burst is asked for.
 */
static uint32_t mcasp_sers_for(const struct dai_ti_mcasp_cfg *c,
			       const struct dai_ti_mcasp_data *d,
			       enum dai_dir dir, uint32_t channels)
{
	const uint8_t want = (dir == DAI_DIR_TX) ? 1U : 2U;
	uint32_t slots_per_ser = mcasp_slots_for(c, d, dir);
	uint32_t n_ser = 0U;

	for (uint32_t i = 0U; i < c->n_ser && i < ARRAY_SIZE(c->ser_dir); i++) {
		if (c->ser_dir[i] == want) {
			n_ser++;
		}
	}
	return mcasp_sers_needed(n_ser, slots_per_ser, channels);
}

static int mcasp_hw_init(const struct dai_ti_mcasp_cfg *c,
			 struct dai_ti_mcasp_data *d, uint32_t rate,
			 uint32_t channels, uint32_t slot_bits, bool rx_live,
			 uint32_t period_bytes)
{
	uint32_t slots_per_ser, need_ser, n_tx = 0U, bitclk, aux_hz, tx_ext;
	uint32_t total_div, hclk_div, aclk_div, xssz, xmask, pdir, act;
	uint32_t tx_slot_mask;

	if (c == NULL) {
		return -EINVAL;
	}
	g_mcasp_cfg_active = c;

	slots_per_ser = mcasp_slots_for(c, d, DAI_DIR_TX);
	slot_bits = mcasp_slot_width_for(d, DAI_DIR_TX, slot_bits);
	if (rate == 0U || slots_per_ser == 0U ||
	(slot_bits != 16U && slot_bits != 24U && slot_bits != 32U)) {
		return -EINVAL;
	}
	/* A count up to the slot count is served on one serializer with the tail slots
	 * left inactive. Beyond that it must divide evenly across serializers.
	 */
	if (channels == 0U) {
		return -EINVAL;
	}
	if (channels >= slots_per_ser && (channels % slots_per_ser) != 0U) {
		return -EINVAL;
	}
	if (channels < slots_per_ser) {
		need_ser = 1U;
		tx_slot_mask = (1U << channels) - 1U;
	} else {
		need_ser = channels / slots_per_ser;
		tx_slot_mask = (slots_per_ser >= 32U) ? 0xFFFFFFFFU
						      : ((1U << slots_per_ser) - 1U);
	}
	for (uint32_t i = 0; i < c->n_ser; i++) {
		if (c->ser_dir[i] == 1U) {
			n_tx++;
		}
	}
	if (need_ser > n_tx) {
		return -EINVAL;
	}

	/* Bit clock and dividers come from the granted AUX rate, never the requested one.
	 */
	bitclk = rate * slot_bits * slots_per_ser;
	tx_ext = mcasp_ext_hclk(d, DAI_DIR_TX);
	aux_hz = (tx_ext != 0U) ? tx_ext : d->aux_hz;
	if (aux_hz == 0U || bitclk == 0U) {
		LOG_ERR("no granted AUX clock (%u) or zero bitclk", aux_hz);
		return -EINVAL;
	}
	total_div = (aux_hz + bitclk / 2U) / bitclk;
	if (total_div == 0U) {
		total_div = 1U;
	}
	d->tx_frame_div = total_div * slot_bits * slots_per_ser;
	if (tx_ext != 0U) {
		/* AHCLKX is the pin. Only the ACLKX divider applies */
		hclk_div = 1U;
		aclk_div = (total_div > 32U) ? 32U : total_div;
	} else if (total_div <= 4096U) {
		hclk_div = total_div;
		aclk_div = 1U;
	} else {
		aclk_div = (total_div + 4095U) / 4096U;
		if (aclk_div > 32U) {
			aclk_div = 32U;
		}
		hclk_div = total_div / aclk_div;
		if (hclk_div == 0U) {
			hclk_div = 1U;
		}
	}

	xssz  = (slot_bits / 2U) - 1U;                 /* TRM XSSZ: 16b=0x7 24b=0xB 32b=0xF */
	xmask = (slot_bits == 32U) ? 0xFFFFFFFFU : ((1U << slot_bits) - 1U);

	pdir = BIT(PIN_BIT_ACLKX) | (tx_ext != 0U ? 0U : BIT(PIN_BIT_AHCLKX)) | BIT(PIN_BIT_AFSX) |
	   BIT(PIN_BIT_ACLKR) | (mcasp_ext_hclk(d, DAI_DIR_RX) != 0U ? 0U : BIT(PIN_BIT_AHCLKR));
	act = 0U;
	for (uint32_t i = 0; i < c->n_ser; i++) {
		if (c->ser_dir[i] == 1U && act < need_ser) {
			pdir |= (1U << i);
			act++;
		}
	}

	d->tx_active_sers = need_ser;

	LOG_DBG("TXGEO: ch=%u slots/ser=%u need=%u ntx=%u wire_slots=%u dt_slots=%u\n",
	   channels, slots_per_ser, need_ser, n_tx,
	   (unsigned int)d->wire_dir[DAI_DIR_TX].tdm_slots, (unsigned int)c->tdm_slots);
	LOG_INF("McASP @%lx: rate=%u ch=%u slot=%ub slots/ser=%u sers=%u/%u "
	    "bitclk=%u aux=%u div=%u (ahclk=%u aclk=%u)",
	    (unsigned long)c->base, rate, channels, slot_bits, slots_per_ser,
	    need_ser, n_tx, bitclk, aux_hz, total_div, hclk_div, aclk_div);

	if (!mcasp_module_is_on(c, MCASP_DIAG_SETTLE_INIT)) {
		return -EIO;
	}

	if (rx_live) {
		mcasp_reg_write(DAVINCI_MCASP_GBLCTLX_REG, 0x00000000);
	} else {
		mcasp_reg_write(DAVINCI_MCASP_GBLCTL_REG, 0x00000000);
		mcasp_reg_write(DAVINCI_MCASP_GBLCTLX_REG, 0x00000000);
	}
	mcasp_dsb();

	mcasp_reg_write(DAVINCI_MCASP_PWREMUMGT_REG, MCASP_FREE);

	d->tx_numevt = mcasp_numevt(c->tx_num_evt, need_ser, period_bytes);
	mcasp_reg_write(DAVINCI_MCASP_WFIFOCTL_REG,
		    NUMEVT(d->tx_numevt) | NUMDMA(need_ser));

	/* mcasp_common_hw_param/mcasp_i2s_hw_param are strictly per-direction. The
	 * playback path never programs an RX register. RX belongs to rx_config.
	 */

	mcasp_reg_write(DAVINCI_MCASP_TXMASK_REG,  xmask);
	mcasp_reg_write(DAVINCI_MCASP_TXFMT_REG,
			FSXDLY(mcasp_fmt_data_delay(d->tx_link.fmt)) | TXORD | TXSSZ(xssz));
	mcasp_reg_write(DAVINCI_MCASP_TXFMCTL_REG, FSXMOD(slots_per_ser) |
			(d->tx_link.cpu_fs_master ? AFSXE : 0U) |
			(mcasp_fmt_word_fs(d->tx_link.fmt) ? FSXDUR : 0U) |
			(mcasp_fs_pol_falling(&d->tx_link) ? FSXPOL : 0U));
	mcasp_reg_write(DAVINCI_MCASP_AHCLKXCTL_REG,
			(tx_ext != 0U) ? 0U : (AHCLKXE | AHCLKXDIV((hclk_div - 1U) & 0xFFFU)));
/* ACLKXPOL set = normal bit clock, cleared = bitclock-inversion */
	mcasp_reg_write(DAVINCI_MCASP_ACLKXCTL_REG,
			(d->tx_link.bclk_inv ? 0U : ACLKXPOL) |
			(c->async_mode ? TX_ASYNC : 0U) |
			(d->tx_link.cpu_bclk_master ? ACLKXE : 0U) |
			ACLKXDIV((aclk_div - 1U) & 0x1FU));
	/* FSXMOD keeps the full frame length. TXTDM enables only the slots that
	 * carry data, so a sub-slot count leaves the tail slots inactive.
	 */
	mcasp_reg_write(DAVINCI_MCASP_TXTDM_REG,   tx_slot_mask);

	/* What an idle serializer drives, from the dismod DT property. DISMOD_LOW when
	 * absent. Three-stating floats the DIN of every codec on an unused jack.
	 */
	uint32_t dismod = (c->dismod == 0U || c->dismod == 2U || c->dismod == 3U) ?
		      DISMOD_VAL(c->dismod) : DISMOD_LOW;

	act = 0U;
	for (uint32_t i = 0; i < c->n_ser && i < 16U; i++) {
		uint32_t v = dismod;

		if (c->ser_dir[i] == 2U) {
			continue;
		}
		if (c->ser_dir[i] == 1U && act < need_ser) {
			v |= MODE(1);
			act++;
		}
		mcasp_reg_write(DAVINCI_MCASP_XRSRCTL_REG(i), v);
	}
	mcasp_reg_write(DAVINCI_MCASP_PFUNC_REG, 0x00000000U);
	/* pdir holds only this direction's bits. Overwriting would clear the RX pin
	 * directions, which one mask covers across both streams.
	 */
	mcasp_reg_write(DAVINCI_MCASP_PDIR_REG, mcasp_reg_read(DAVINCI_MCASP_PDIR_REG) | pdir);
	if (!rx_live) {
		mcasp_reg_write(DAVINCI_MCASP_TXDITCTL_REG, 0x00000000U);
		mcasp_reg_write(DAVINCI_MCASP_LBCTL_REG, 0x00000000U);
		mcasp_reg_write(DAVINCI_MCASP_AMUTE_REG, 0x00000000U);
	}
	mcasp_dsb();

	if (mcasp_gblctl_set(c, TXHCLKRST) != 0 ||
	mcasp_gblctl_set(c, TXCLKRST) != 0) {
		return -EIO;
	}
	mcasp_dsb();

	mcasp_reg_write(DAVINCI_MCASP_TXSTAT_REG, MCASP_STAT_CLR);
	mcasp_reg_write(DAVINCI_MCASP_XEVTCTL_REG, DATDMA_DIS);
	mcasp_dsb();

	return 0;
}

static void mcasp_rx_base(const struct dai_ti_mcasp_cfg *c, const struct dai_ti_mcasp_data *d)
{
	mcasp_reg_write(DAVINCI_MCASP_GBLCTL_REG, 0x00000000);
	mcasp_reg_write(DAVINCI_MCASP_GBLCTLX_REG, 0x00000000);
	mcasp_dsb();
	mcasp_reg_write(DAVINCI_MCASP_PWREMUMGT_REG, MCASP_FREE);
	mcasp_reg_write(DAVINCI_MCASP_PFUNC_REG, 0x00000000U);
	mcasp_reg_write(DAVINCI_MCASP_PDIR_REG,  BIT(PIN_BIT_ACLKX) | BIT(PIN_BIT_AHCLKX) |
		    BIT(PIN_BIT_AFSX) | BIT(PIN_BIT_ACLKR) | BIT(PIN_BIT_AFSR));
/* an external AHCLK pin stays an input */
	if (mcasp_ext_hclk(d, DAI_DIR_RX) != 0U) {
		mcasp_reg_write(DAVINCI_MCASP_PDIR_REG,
				mcasp_reg_read(DAVINCI_MCASP_PDIR_REG) & ~BIT(PIN_BIT_AHCLKR));
	}
	if (mcasp_ext_hclk(d, DAI_DIR_TX) != 0U) {
		mcasp_reg_write(DAVINCI_MCASP_PDIR_REG,
				mcasp_reg_read(DAVINCI_MCASP_PDIR_REG) & ~BIT(PIN_BIT_AHCLKX));
	}
	mcasp_reg_write(DAVINCI_MCASP_TXDITCTL_REG, 0x00000000U);
	mcasp_reg_write(DAVINCI_MCASP_LBCTL_REG, 0x00000000U);
	mcasp_reg_write(DAVINCI_MCASP_AMUTE_REG, 0x00000000U);
	mcasp_reg_write(DAVINCI_MCASP_RXSTAT_REG, MCASP_STAT_CLR);
	mcasp_dsb();
}

/*
 *  mcasp_rx_config. Program the RX section and release the RX clocks, from the
 *  granted AUX rate: rounded divider, AHCLKR<=4096, ACLKR<=32. RX runs ASYNC on
 *  its own clock. Serializer and frame-sync release follows in mcasp_rx_start.
 */
static int mcasp_rx_config(const struct dai_ti_mcasp_cfg *c,
			   struct dai_ti_mcasp_data *d, uint32_t rate,
			   uint32_t channels, uint32_t slot_bits,
			   uint32_t period_bytes)
{
	uint32_t n_rx = 0U, slots, bitclk, aux_hz, rx_ext;
	uint32_t total_div, hclk_div, aclk_div, rssz, rmask, rrot;

	if (c == NULL) {
		return -EINVAL;
	}
	slot_bits = mcasp_slot_width_for(d, DAI_DIR_RX, slot_bits);
	for (uint32_t i = 0; i < c->n_ser; i++) {
		if (c->ser_dir[i] == 2U) {
			n_rx++;
		}
	}
	if (rate == 0U || channels == 0U || n_rx == 0U ||
	(slot_bits != 16U && slot_bits != 24U && slot_bits != 32U)) {
		return -EINVAL;
	}
	if ((channels % n_rx) != 0U) {
		return -EINVAL;
	}
	slots = mcasp_slots_for(c, d, DAI_DIR_RX);
	if (slots == 0U || slots > 32U || channels > (n_rx * slots)) {
		return -EINVAL;
	}

	bitclk = rate * slot_bits * slots;
	rx_ext = mcasp_ext_hclk(d, DAI_DIR_RX);
	aux_hz = (rx_ext != 0U) ? rx_ext : d->aux_hz;
	if (aux_hz == 0U || bitclk == 0U) {
		return -EINVAL;
	}
	total_div = (aux_hz + bitclk / 2U) / bitclk;
	if (total_div == 0U) {
		total_div = 1U;
	}
	if (d->tx_frame_div != 0U && rx_ext == 0U) {
		uint32_t rx_bits_per_frame = slot_bits * slots;
		uint32_t derived = d->tx_frame_div / rx_bits_per_frame;

		if (derived != 0U && derived * rx_bits_per_frame == d->tx_frame_div) {
			total_div = derived;
		}
	}
	if (rx_ext != 0U) {
		hclk_div = 1U;
		aclk_div = (total_div > 32U) ? 32U : total_div;
	} else if (total_div <= 4096U) {
		hclk_div = total_div;
		aclk_div = 1U;
	} else {
		aclk_div = (total_div + 4095U) / 4096U;
		if (aclk_div > 32U) {
			aclk_div = 32U;
		}
		hclk_div = total_div / aclk_div;
		if (hclk_div == 0U) {
			hclk_div = 1U;
		}
	}
	rssz  = (slot_bits / 2U) - 1U;
	rrot  = (32U - slot_bits) / 4U;
	/* RXROT rotates, so a narrow slot wraps the buffer's upper bits into the word.
	 * Mask everything outside the rotated sample.
	 */
	rmask = (slot_bits >= 32U) ? 0xFFFFFFFFU
	    : (((1U << slot_bits) - 1U) << (rrot * 4U));

	d->rx_active_sers = n_rx;

	LOG_INF("McASP RX: rate=%u ch=%u slot=%ub slots=%u sers=%u bitclk=%u "
	    "div=%u (ahclkr=%u aclkr=%u)", rate, channels, slot_bits, slots,
	    n_rx, bitclk, total_div, hclk_div, aclk_div);

	/*
	 *  RX registers (offsets = TX - 0x40). AFSRCTL=(slots<<7)|FSRM makes RX frame
	 *  master. AHCLKRCTL is written before ACLKRCTL so the high clock settles first.
	 */
	mcasp_reg_write(DAVINCI_MCASP_RXMASK_REG, rmask);
	mcasp_reg_write(DAVINCI_MCASP_RXFMT_REG,
		    FSRDLY(mcasp_fmt_data_delay(d->rx_link.fmt)) | RXORD | RXSSZ(rssz) |
		    RXROT(rrot));
	mcasp_reg_write(DAVINCI_MCASP_RXFMCTL_REG, FSRMOD(slots) |
		    (d->rx_link.cpu_fs_master ? AFSRE : 0U) |
		    (mcasp_fmt_word_fs(d->rx_link.fmt) ? FSRDUR : 0U) |
		    (mcasp_fs_pol_falling(&d->rx_link) ? FSRPOL : 0U));
	mcasp_reg_write(DAVINCI_MCASP_AHCLKRCTL_REG,
		    (rx_ext != 0U) ? 0U : (AHCLKRE | AHCLKRDIV((hclk_div - 1U) & 0xFFFU)));
	/* the capture link's bitclock-inversion clears ACLKRPOL (the pcm6240 needs it) */
	mcasp_reg_write(DAVINCI_MCASP_ACLKRCTL_REG,
		    (d->rx_link.bclk_inv ? 0U : ACLKRPOL) |
		    (d->rx_link.cpu_bclk_master ? ACLKRE : 0U) |
		    ACLKRDIV((aclk_div - 1U) & 0x1FU));
	{
		uint32_t act_slots = (channels < slots) ? channels : slots;

		mcasp_reg_write(DAVINCI_MCASP_RXTDM_REG, (act_slots >= 32U) ? 0xFFFFFFFFU
				: ((1U << act_slots) - 1U));
	}
	/*
	 *  RCLKCHK stays at its reset value. Arming the RX clock check breaks duplex
	 *  capture. RNUMDMA = active RX serializers; RNUMEVT shrinks in serializer steps
	 *  from the DT ceiling.
	 */
	d->rx_numevt = mcasp_numevt(c->rx_num_evt, n_rx, period_bytes);
	mcasp_reg_write(DAVINCI_MCASP_RFIFOCTL_REG, NUMEVT(d->rx_numevt) | NUMDMA(n_rx));
	for (uint32_t i = 0; i < c->n_ser && i < 16U; i++) {
		if (c->ser_dir[i] == 2U) {
			mcasp_reg_write(DAVINCI_MCASP_XRSRCTL_REG(i), MODE(2));
		}
	}
	mcasp_reg_write(DAVINCI_MCASP_PDIR_REG, mcasp_reg_read(DAVINCI_MCASP_PDIR_REG) |
			BIT(PIN_BIT_ACLKR) | BIT(PIN_BIT_AFSR) |
			(rx_ext != 0U ? 0U : BIT(PIN_BIT_AHCLKR)));
	mcasp_reg_write(DAVINCI_MCASP_EVTCTLR_REG, 0U);
	mcasp_reg_write(DAVINCI_MCASP_REVTCTL_REG, DATDMA_DIS);
	mcasp_dsb();
	if (mcasp_gblctl_set(c, RXHCLKRST) != 0 ||
	mcasp_gblctl_set(c, RXCLKRST) != 0) {
		return -EIO;
	}
	mcasp_dsb();
	return 0;
}

volatile uint32_t g_mcasp_relatch[4] __used __aligned(64);

static int mcasp_rx_start(const struct dai_ti_mcasp_cfg *c,
			  struct dai_ti_mcasp_data *d)
{
	if (c == NULL) {
		return -EINVAL;
	}
	/* Heal a config-time latch loss (clock transient). Clocks must be
	 * released BEFORE the state machine, or the wire stays dead.
	 */
	if ((mcasp_reg_read(DAVINCI_MCASP_GBLCTL_REG) & (RXCLKRST | RXHCLKRST))
	!= (RXCLKRST | RXHCLKRST)) {
		g_mcasp_relatch[0]++;
		if (mcasp_gblctl_set(c, RXHCLKRST) != 0 ||
		    mcasp_gblctl_set(c, RXCLKRST) != 0) {
			g_mcasp_relatch[1]++;
		}
	}
	mcasp_reg_write(DAVINCI_MCASP_RXSTAT_REG, MCASP_XSTAT_ARM);
	mcasp_dsb();
	if (d->rx_numevt) {
		mcasp_reg_write(DAVINCI_MCASP_RFIFOCTL_REG,
				NUMEVT(d->rx_numevt) | NUMDMA(d->rx_active_sers));
		mcasp_dsb();
	}
	mcasp_reg_write(DAVINCI_MCASP_RFIFOCTL_REG,
		    (d->rx_numevt ? FIFO_ENABLE : 0U) |
		    NUMEVT(d->rx_numevt) | NUMDMA(d->rx_active_sers));
	mcasp_dsb();
	mcasp_reg_write(DAVINCI_MCASP_REVTCTL_REG, 0x00000000U);
	mcasp_dsb();
	/* Every GBLCTL bit is checked and a failure aborts the start. A bit that does
	 * not latch leaves capture misaligned or dead.
	 */
	if (mcasp_gblctl_set(c, RXSERCLR) != 0 ||
	    mcasp_gblctl_set(c, RXSERCLR) != 0 ||
	    mcasp_gblctl_set(c, RXSMRST) != 0 ||
	    mcasp_gblctl_set(c, RXFSRST) != 0) {
		return -EIO;
	}
	mcasp_dsb();
	/* RFIFOSTS is the residual word count. A non-empty FIFO at start offsets
	 * the first frame and rotates every channel after it
	 */
	LOG_DBG("RXSTATE: GBL=%08x PDIR=%08x RTDM=%08x RFMT=%08x SR14=%08x RSTAT=%08x RLVL=%08x\n",
	   mcasp_reg_read(DAVINCI_MCASP_GBLCTL_REG),
	   mcasp_reg_read(DAVINCI_MCASP_PDIR_REG),
	   mcasp_reg_read(DAVINCI_MCASP_RXTDM_REG),
	   mcasp_reg_read(DAVINCI_MCASP_RXFMT_REG),
	   mcasp_reg_read(DAVINCI_MCASP_XRSRCTL_REG(14)),
	   mcasp_reg_read(DAVINCI_MCASP_RXSTAT_REG),
	   mcasp_reg_read(DAVINCI_MCASP_RFIFOSTS_REG));
	/* A fixed delay, not a wait on a condition, and the mechanism is not understood:
	 * removing it breaks the 32k->48k starts; 48k duplex does not need it.
	 */
	{
		uint32_t rs0 = mcasp_reg_read(DAVINCI_MCASP_RXSTAT_REG);
		volatile uint32_t spin;
		uint32_t rs1;

		for (spin = 0U; spin < 200000U; spin++) {
		}
		rs1 = mcasp_reg_read(DAVINCI_MCASP_RXSTAT_REG);
		LOG_DBG("RXCLK: ACLKR=%08x AHCLKR=%08x AFSR=%08x RSTAT %08x->%08x clocked=%d\n",
			mcasp_reg_read(DAVINCI_MCASP_ACLKRCTL_REG),
			mcasp_reg_read(DAVINCI_MCASP_AHCLKRCTL_REG),
			mcasp_reg_read(DAVINCI_MCASP_RXFMCTL_REG),
			rs0, rs1, (int)(rs1 != rs0));
	}
	if (c->has_rx_irq) {
		mcasp_reg_write(DAVINCI_MCASP_EVTCTLR_REG, RSTAT_ROVRN);
	}
	LOG_INF("McASP RX started (SM+FS released)");
	return 0;
}

static void mcasp_rx_quiesce(const struct dai_ti_mcasp_cfg *c)
{
	BC(20);
	if (c == NULL) {
		return;
	}
	mcasp_reg_write(DAVINCI_MCASP_EVTCTLR_REG, 0U);
	mcasp_reg_write(DAVINCI_MCASP_REVTCTL_REG, DATDMA_DIS);
	mcasp_dsb();
	mcasp_reg_write(DAVINCI_MCASP_GBLCTLR_REG, 0x00000000U);
	mcasp_reg_write(DAVINCI_MCASP_RXSTAT_REG, MCASP_STAT_CLR);
	if (mcasp_reg_read(DAVINCI_MCASP_RFIFOCTL_REG) & FIFO_ENABLE) {
		mcasp_reg_write(DAVINCI_MCASP_RFIFOCTL_REG,
				mcasp_reg_read(DAVINCI_MCASP_RFIFOCTL_REG) & ~FIFO_ENABLE);
	}
	mcasp_dsb();
	BC(21);
}

/*
 *  mcasp_tx_quiesce. Stop only the TX side, through the XGBLCTL per-side alias.
 *  A full-GBLCTL stop under a live RX disturbs the capture leg. The alias write
 *  leaves the RX bits untouched.
 */
static void mcasp_tx_quiesce(const struct dai_ti_mcasp_cfg *c)
{
	BC(22);
	if (c == NULL) {
		return;
	}
	mcasp_reg_write(DAVINCI_MCASP_EVTCTLX_REG, 0U);
	mcasp_reg_write(DAVINCI_MCASP_XEVTCTL_REG, DATDMA_DIS);
	mcasp_dsb();
	mcasp_reg_write(DAVINCI_MCASP_GBLCTLX_REG, 0x00000000);
	mcasp_reg_write(DAVINCI_MCASP_TXSTAT_REG, MCASP_STAT_CLR);
	if (mcasp_reg_read(DAVINCI_MCASP_WFIFOCTL_REG) & FIFO_ENABLE) {
		mcasp_reg_write(DAVINCI_MCASP_WFIFOCTL_REG,
				mcasp_reg_read(DAVINCI_MCASP_WFIFOCTL_REG) & ~FIFO_ENABLE);
	}
	mcasp_dsb();
	BC(23);
}

/*
 *  mcasp_tx_start: release the FIFO, serializers, state machine and frame sync.
 *  Called after the BCDMA channel is armed, so the DMA primes the FIFO before the
 *  wire starts.
 */
static int mcasp_tx_start(const struct dai_ti_mcasp_cfg *c,
			  struct dai_ti_mcasp_data *d)
{
	if (c == NULL) {
		return -EINVAL;
	}
	uint32_t cnt;

	mcasp_reg_write(DAVINCI_MCASP_TXSTAT_REG, MCASP_XSTAT_ARM);
	mcasp_dsb();
	/* Clear FIFO_ENABLE before setting it, so the AFIFO starts empty rather than
	 * inheriting the previous stream's contents.
	 */
	if (d->tx_numevt) {
		mcasp_reg_write(DAVINCI_MCASP_WFIFOCTL_REG,
				NUMEVT(d->tx_numevt) | NUMDMA(d->tx_active_sers));
		mcasp_dsb();
	}
	mcasp_reg_write(DAVINCI_MCASP_WFIFOCTL_REG,
			(d->tx_numevt ? FIFO_ENABLE : 0U) |
			NUMEVT(d->tx_numevt) | NUMDMA(d->tx_active_sers));
	mcasp_dsb();
	mcasp_reg_write(DAVINCI_MCASP_XEVTCTL_REG, 0x00000000U);
	mcasp_dsb();

/* a GBLCTL bit that does not latch aborts the start */
	if (mcasp_gblctl_set(c, TXSERCLR) != 0) {
		return -EIO;
	}

	/* Prime the FIFO before releasing the transmitter. The AFIFO requests data as
	 * soon as it is enabled, and releasing against an empty FIFO underruns.
	 */
	if (d->tx_numevt) {
		cnt = 0U;
		while (mcasp_reg_read(DAVINCI_MCASP_WFIFOSTS_REG) < d->tx_numevt &&
		       cnt < MCASP_TX_PRIME_SPINS) {
			cnt++;
		}
		if (cnt >= MCASP_TX_PRIME_SPINS) {
			LOG_ERR("TX AFIFO never filled (WFIFOSTS %u < %u): DMA not delivering",
				mcasp_reg_read(DAVINCI_MCASP_WFIFOSTS_REG), d->tx_numevt);
			return -EIO;
		}
	}

/* wait for XRDATA to clear before releasing the serializers */
	cnt = 0U;
	while ((mcasp_reg_read(DAVINCI_MCASP_TXSTAT_REG) & XSTAT_XRDATA) &&
	   cnt < MCASP_TX_PRIME_SPINS) {
		cnt++;
	}

	if (mcasp_gblctl_set(c, TXSMRST) != 0 || mcasp_gblctl_set(c, TXFSRST) != 0) {
		return -EIO;
	}
	g_mcasp_tx_underrun_pending = 0U;
	if (c->has_tx_irq) {
		mcasp_reg_write(DAVINCI_MCASP_EVTCTLX_REG, XSTAT_XUNDRN);
	}
	mcasp_dsb();
	LOG_DBG("TXSTATE: GBL=%08x PDIR=%08x XTDM=%08x WFIFO=%08x "
		"SR=%08x/%08x/%08x/%08x XSTAT=%08x\n",
	   mcasp_reg_read(DAVINCI_MCASP_GBLCTL_REG),
	   mcasp_reg_read(DAVINCI_MCASP_PDIR_REG),
	   mcasp_reg_read(DAVINCI_MCASP_TXTDM_REG),
	   mcasp_reg_read(DAVINCI_MCASP_WFIFOCTL_REG),
	   mcasp_reg_read(DAVINCI_MCASP_XRSRCTL_REG(0)),
	   mcasp_reg_read(DAVINCI_MCASP_XRSRCTL_REG(1)),
	   mcasp_reg_read(DAVINCI_MCASP_XRSRCTL_REG(3)),
	   mcasp_reg_read(DAVINCI_MCASP_XRSRCTL_REG(4)),
	   mcasp_reg_read(DAVINCI_MCASP_TXSTAT_REG));
	LOG_INF("McASP TX started (SM+FS released)");
	return 0;
}

static void mcasp_hw_stop(const struct dai_ti_mcasp_cfg *c)
{
	if (c == NULL) {
		return;
	}
	if (!mcasp_module_is_on(c, MCASP_DIAG_SETTLE_STOP)) {
		return;
	}
	LOG_INF("Stopping McASP @%lx", (unsigned long)c->base);

	mcasp_reg_write(DAVINCI_MCASP_GBLCTL_REG, 0x00000000);
	mcasp_reg_write(DAVINCI_MCASP_GBLCTLX_REG, 0x00000000);
	mcasp_reg_write(DAVINCI_MCASP_WFIFOCTL_REG,
		    mcasp_reg_read(DAVINCI_MCASP_WFIFOCTL_REG) & ~FIFO_ENABLE);
	mcasp_reg_write(DAVINCI_MCASP_RFIFOCTL_REG,
		    mcasp_reg_read(DAVINCI_MCASP_RFIFOCTL_REG) & ~FIFO_ENABLE);
	mcasp_reg_write(DAVINCI_MCASP_PDIR_REG,
		    mcasp_reg_read(DAVINCI_MCASP_PDIR_REG) & 0xFFFF0000U);
	mcasp_dsb();
}

/* Single instance, cached for callers in the DMA layer that carry no DAI device */
static const struct device *g_mcasp_dev;

/* The TX AFIFO event size in force for this stream, or 0 when bypassed. The PDMA
 * element count must equal it, or the event is never satisfied.
 */
uint32_t dai_ti_mcasp_tx_numevt(void)
{
	const struct dai_ti_mcasp_data *d;

	if (g_mcasp_dev == NULL) {
		return 0U;
	}
	d = g_mcasp_dev->data;
	return d->tx_numevt;
}

/* Underruns seen and recovered since boot. Read by name from the host. */
volatile uint32_t g_mcasp_tx_underruns;

/*
 *  A latched XUNDRN halts the transmitter until the state machines are reset, and
 *  no McASP interrupt reaches the C7x, so it is polled here. Only while the data
 *  path is already starved.
 */
int dai_ti_mcasp_tx_recover_underrun(void)
{
	const struct dai_ti_mcasp_cfg *c;
	struct dai_ti_mcasp_data *d;

	if (g_mcasp_dev == NULL) {
		return 0;
	}
	c = g_mcasp_dev->config;
	d = g_mcasp_dev->data;

	if ((mcasp_reg_read(DAVINCI_MCASP_TXSTAT_REG) & XSTAT_XUNDRN) == 0U &&
	    g_mcasp_tx_underrun_pending == 0U) {
		return 0;
	}
	if (g_mcasp_tx_underrun_pending != 0U) {
		g_mcasp_tx_underrun_pending = 0U;
	} else {
		g_mcasp_tx_underruns++;
	}
	mcasp_tx_quiesce(c);
	if (mcasp_tx_start(c, d) != 0) {
		LOG_ERR("TX restart after underrun failed");
	}
	return 1;
}


static int mcasp_clk_up(const struct device *dev)
{
	const struct dai_ti_mcasp_cfg *c = dev->config;
	struct dai_ti_mcasp_data *d = dev->data;
	clock_control_subsys_t sys = (clock_control_subsys_t)&c->clk;
	uint32_t parent = c->clk_parent, rate = 0U;
	uint64_t clk_rate64 = c->clk_rate;
	int rc;

	/*
	 *  Every clock step is checked. A refused SET_CLOCK_PARENT or SET_FREQ leaves
	 *  AUXCLK at its default and every derived bit clock wrong.
	 */
	/* Idle the clock before re-parenting it. */
	(void)tisci_cmd_idle_clock(dmsc, c->clk.dev_id, c->clk.clk_id);
	rc = tisci_cmd_clk_set_parent(dmsc, c->clk.dev_id, c->clk.clk_id,
				      parent);
	if (rc != 0) {
		LOG_ERR("probe: SET_CLOCK_PARENT(%u) failed: %d", parent, rc);
	}
	rc = clock_control_set_rate(c->clk_dev, sys,
			(clock_control_subsys_rate_t)&clk_rate64);
	if (rc != 0) {
		LOG_ERR("probe: SET_FREQ(%u) NAKed: %d", c->clk_rate, rc);
	}
	rc = tisci_cmd_get_clock(dmsc, c->clk.dev_id, c->clk.clk_id,
				 false, true, false);
	if (rc != 0) {
		LOG_ERR("probe: SET_CLOCK(REQ) failed: %d", rc);
	}
	if (clock_control_get_rate(c->clk_dev, sys, &rate) == 0) {
		d->aux_hz = rate;
	}
	if (d->aux_hz != c->clk_rate) {
		LOG_WRN("probe: granted AUX %u Hz != DT %u Hz", d->aux_hz,
			c->clk_rate);
	}
	d->aclk_hz[0] = d->aux_hz;
	/* the further assigned-clocks entries route the AHCLKX/AHCLKR pin muxes. */
	for (uint8_t i = 1U; i < c->n_aclk && i < MCASP_ACLK_MAX; i++) {
		struct tisci_clock_config ck = { .dev_id = c->clk.dev_id, .clk_id = c->aclk_id[i] };
		uint64_t want = c->aclk_rate[i];

		(void)tisci_cmd_idle_clock(dmsc, ck.dev_id, ck.clk_id);
		rc = tisci_cmd_clk_set_parent(dmsc, ck.dev_id, ck.clk_id, c->aclk_parent[i]);
		if (rc != 0) {
			LOG_ERR("probe: clk %u SET_CLOCK_PARENT(%u) failed: %d", ck.clk_id,
				c->aclk_parent[i], rc);
		}
		if (want != 0U) {
			rc = clock_control_set_rate(c->clk_dev, (clock_control_subsys_t)&ck,
						    (clock_control_subsys_rate_t)&want);
			if (rc != 0) {
				LOG_ERR("probe: clk %u SET_FREQ(%u) NAKed: %d", ck.clk_id,
					c->aclk_rate[i], rc);
			}
		}
		(void)tisci_cmd_get_clock(dmsc, ck.dev_id, ck.clk_id, false, true, false);
		rate = 0U;
		(void)clock_control_get_rate(c->clk_dev, (clock_control_subsys_t)&ck, &rate);
		d->aclk_hz[i] = rate;
		if (rate != c->aclk_rate[i]) {
			LOG_WRN("probe: clk %u granted %u Hz != DT %u Hz", ck.clk_id, rate,
				c->aclk_rate[i]);
		}
	}
	d->clk_acquired = (d->aux_hz != 0U);
	return d->clk_acquired ? 0 : -EIO;
}

static void mcasp_clk_down(const struct device *dev)
{
	const struct dai_ti_mcasp_cfg *c = dev->config;
	struct dai_ti_mcasp_data *d = dev->data;

	if (!d->clk_acquired) {
		return;
	}
	for (uint8_t i = 1U; i < c->n_aclk && i < MCASP_ACLK_MAX; i++) {
		(void)tisci_cmd_put_clock(dmsc, c->clk.dev_id, c->aclk_id[i]);
	}
	(void)tisci_cmd_put_clock(dmsc, c->clk.dev_id, c->clk.clk_id);
	d->aux_hz = 0U;
	d->clk_acquired = false;
}

/* Register state is programmed per stream, so power transitions need no save or restore. */
static int dai_ti_mcasp_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return mcasp_clk_up(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		mcasp_clk_down(dev);
		return 0;
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;
	default:
		return -ENOTSUP;
	}
}

uint32_t dai_ti_mcasp_tx_burst(const struct device *dev, uint32_t channels)
{
	const struct dai_ti_mcasp_cfg *c = dev->config;
	struct dai_ti_mcasp_data *d = dev->data;
	uint32_t nv = d->tx_numevt;

	if (nv == 0U) {
		nv = mcasp_numevt(c->tx_num_evt, d->tx_active_sers, 0U);
	}
	if (nv == 0U) {
		nv = mcasp_sers_for(c, d, DAI_DIR_TX, channels);
	}
	return nv;
}

uint32_t dai_ti_mcasp_rx_burst(const struct device *dev, uint32_t channels)
{
	const struct dai_ti_mcasp_cfg *c = dev->config;
	struct dai_ti_mcasp_data *d = dev->data;
	uint32_t nv = d->rx_numevt;

	if (nv == 0U) {
		nv = mcasp_numevt(c->rx_num_evt, d->rx_active_sers, 0U);
	}
	if (nv == 0U) {
		nv = mcasp_sers_for(c, d, DAI_DIR_RX, channels);
	}
	return nv;
}

static int dai_ti_mcasp_config_set(const struct device *dev,
				   const struct dai_config *cfg,
				   const void *bespoke_cfg, size_t size)
{
	struct dai_ti_mcasp_data *d = dev->data;
	const struct dai_ti_mcasp_blob *blob =
		(size >= sizeof(*blob)) ? bespoke_cfg : NULL;

	if (cfg == NULL) {
		return -EINVAL;
	}
	d->cfg = *cfg;
	d->wire.tdm_slots = (blob != NULL) ? blob->tdm_slots : 0U;
	d->wire.tdm_slot_width = (blob != NULL) ? blob->tdm_slot_width : 0U;
	LOG_DBG("MCASP CFG: ch=%u rate=%u wire_slots=%u wire_sw=%u blob=%d\n",
	       (unsigned int)cfg->channels, (unsigned int)cfg->rate,
	       (unsigned int)d->wire.tdm_slots, (unsigned int)d->wire.tdm_slot_width,
	       (int)(blob != NULL));
	return 0;
}

static int dai_ti_mcasp_config_get(const struct device *dev,
				   struct dai_config *cfg, enum dai_dir dir)
{
	struct dai_ti_mcasp_data *d = dev->data;

	if (cfg == NULL) {
		return -EINVAL;
	}
	if ((dir == DAI_DIR_TX || dir == DAI_DIR_RX) && d->cfg_dir_valid[dir]) {
		*cfg = d->cfg_dir[dir];
	} else {
		*cfg = d->cfg;
	}
	cfg->type = DAI_TI_MCASP;
	cfg->dai_index = 0;
	/* 0 means "varies" to dai_verify_params(), which is what lets one McASP
	 * serve 1..8 channels rather than only the count last latched
	 */
	cfg->channels = 0;
	return 0;
}

static const struct dai_properties *
dai_ti_mcasp_get_properties(const struct device *dev, enum dai_dir dir,
			    int stream_id)
{
	const struct dai_ti_mcasp_cfg *c = dev->config;
	struct dai_ti_mcasp_data *d = dev->data;
	struct dai_properties *p;

	ARG_UNUSED(stream_id);
	if (dir == DAI_DIR_TX) {
		p = &d->props[DAI_DIR_TX];
		{
			uint32_t nv = d->tx_numevt;

			if (nv == 0U) {
				nv = mcasp_numevt(c->tx_num_evt,
						  d->tx_active_sers, 0U);
			}
			if (nv == 0U) {
				nv = d->tx_active_sers;
			}
			p->fifo_depth = nv;
		}
		p->dma_hs_id  = c->tx_dma_thread;
		p->stream_id  = (int)d->tx_active_sers;
	} else if (dir == DAI_DIR_RX) {
		p = &d->props[DAI_DIR_RX];
		p->fifo_depth = d->rx_numevt ? d->rx_numevt : d->rx_active_sers;
		p->dma_hs_id  = c->rx_dma_thread;
		p->stream_id  = (int)d->rx_active_sers;
	} else {
		return NULL;
	}
	p->fifo_address = (uint32_t)c->dat_base;
	return p;
}

/*
 *  Playback channel map from the DAI's own geometry. The wire is slot-major, so
 *  position p lands on serializer p%active_ser, slot p/active_ser. Nibble p names
 *  the stream channel for that position, and 0xf mutes it.
 */
uint32_t dai_ti_mcasp_playback_chan_map(const struct device *dev, uint32_t channels)
{
	const struct dai_ti_mcasp_cfg *c;
	struct dai_ti_mcasp_data *d;
	uint32_t slots, avail = 0U, active, wire, p, map = 0U;

	if (dev == NULL || channels == 0U || channels > 8U) {
		return 0U;
	}
	c = dev->config;
	d = dev->data;

	slots = mcasp_slots_for(c, d, DAI_DIR_TX);
	if (slots == 0U) {
		return 0U;
	}
	for (uint32_t i = 0; i < c->n_ser; i++) {
		if (c->ser_dir[i] == 1U) {
			avail++;
		}
	}

	/* odd counts leave the tail slots inactive, so round up */
	active = (channels + slots - 1U) / slots;
	if (active > avail || active <= 1U) {
		return 0U;	/* one serializer is already slot order */
	}
	wire = active * slots;
	if (wire > 8U) {
		return 0U;
	}
	for (p = 0U; p < wire; p++) {
		uint32_t src = (p % active) * slots + (p / active);

		map |= ((src < channels) ? src : 0xfU) << (p * 4U);
	}
	return map;
}

static int dai_ti_mcasp_trigger_inner(const struct device *dev, enum dai_dir dir,
				      enum dai_trigger_cmd cmd,
				      const struct dai_ti_mcasp_cfg *c,
				      struct dai_ti_mcasp_data *d);

static int dai_ti_mcasp_trigger(const struct device *dev, enum dai_dir dir,
				enum dai_trigger_cmd cmd)
{
	const struct dai_ti_mcasp_cfg *c = dev->config;
	struct dai_ti_mcasp_data *d = dev->data;

	int _bc_rc;

	BC(10);
	LOG_DBG("MCASP TRIG: cmd=%d dir=%d tx_act=%d rx_act=%d\n",
	       (int)cmd, (int)dir, (int)d->tx_active, (int)d->rx_active);
	BC(11);
	_bc_rc = dai_ti_mcasp_trigger_inner(dev, dir, cmd, c, d);
	BC(12);
	LOG_DBG("MCASP TRIG done: cmd=%d dir=%d rc=%d\n", (int)cmd, (int)dir, _bc_rc);
	BC(13);
	return _bc_rc;
}

static int dai_ti_mcasp_trigger_inner(const struct device *dev, enum dai_dir dir,
				      enum dai_trigger_cmd cmd,
				      const struct dai_ti_mcasp_cfg *c,
				      struct dai_ti_mcasp_data *d)
{
	ARG_UNUSED(dev);
	switch (cmd) {
	case DAI_TRIGGER_PRE_START:
		if (dir == DAI_DIR_TX) {
			d->cfg_dir[DAI_DIR_TX] = d->cfg;
			d->wire_dir[DAI_DIR_TX] = d->wire;
			d->cfg_dir_valid[DAI_DIR_TX] = true;
			return (mcasp_hw_init(c, d, d->cfg.rate, d->cfg.channels,
					      d->cfg.word_size, d->rx_active,
					      (uint32_t)d->cfg.block_size) != 0) ? -EIO : 0;
		}
		if (dir == DAI_DIR_RX) {
			d->cfg_dir[DAI_DIR_RX] = d->cfg;
			d->wire_dir[DAI_DIR_RX] = d->wire;
			d->cfg_dir_valid[DAI_DIR_RX] = true;
			if (!d->tx_active) {
				mcasp_rx_base(c, d);
			}
			return (mcasp_rx_config(c, d, d->cfg.rate,
						d->cfg.channels,
						d->cfg.word_size,
						(uint32_t)d->cfg.block_size) != 0) ? -EIO : 0;
		}
		return -EINVAL;
	case DAI_TRIGGER_START:
		if (dir == DAI_DIR_TX) {
			int rc = mcasp_tx_start(c, d);

			if (rc != 0) {
				return rc;
			}
			d->tx_active = true;
			return 0;
		}
		if (dir == DAI_DIR_RX) {
			int rx_rc = mcasp_rx_start(c, d);

			if (rx_rc != 0) {
				return rx_rc;
			}
			d->rx_active = true;
			return 0;
		}
		return -EINVAL;
	case DAI_TRIGGER_STOP:
		/* config_get() must not answer for a stream that has stopped:
		 * the latch covers the stream it was taken for, and no longer
		 */
		if (dir == DAI_DIR_TX) {
			mcasp_tx_quiesce(c);
			d->tx_active = false;
			d->cfg_dir_valid[DAI_DIR_TX] = false;
			return 0;
		}
		if (dir == DAI_DIR_RX) {
			mcasp_rx_quiesce(c);
			d->rx_active = false;
			d->cfg_dir_valid[DAI_DIR_RX] = false;
			return 0;
		}
		return -EINVAL;
	case DAI_TRIGGER_PAUSE:
		/* SOF suspends the BCDMA channel the moment this returns, and disabling it while
		 * McASP still drives PSI-L hangs the DMA controller. The active flags stay set.
		 */
		if (dir == DAI_DIR_TX) {
			mcasp_tx_quiesce(c);
			return 0;
		}
		if (dir == DAI_DIR_RX) {
			mcasp_rx_quiesce(c);
			return 0;
		}
		return -EINVAL;
	case DAI_TRIGGER_RESET:
		mcasp_hw_stop(c);
		d->tx_active = false;
		d->rx_active = false;
		d->cfg_dir_valid[DAI_DIR_TX] = false;
		d->cfg_dir_valid[DAI_DIR_RX] = false;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int dai_ti_mcasp_init(const struct device *dev)
{
	const struct dai_ti_mcasp_cfg *c = dev->config;
	struct dai_ti_mcasp_data *d = dev->data;

	g_mcasp_dev = dev;
	mcasp_links_resolve(c, d);
	if (c->has_tx_irq) {
		irq_connect_dynamic(c->tx_irq, c->tx_irq_prio, mcasp_tx_irq, dev, 0U);
		c7x_clec_irq_enable(c->tx_irq);
		irq_enable(c->tx_irq);
	}
	if (c->has_rx_irq) {
		irq_connect_dynamic(c->rx_irq, c->rx_irq_prio, mcasp_rx_irq, dev, 0U);
		c7x_clec_irq_enable(c->rx_irq);
		irq_enable(c->rx_irq);
	}
	return 0;
}

static DEVICE_API(dai, dai_ti_mcasp_api) = {
	.probe          = pm_device_runtime_get,
	.remove         = pm_device_runtime_put,
	.config_set     = dai_ti_mcasp_config_set,
	.config_get     = dai_ti_mcasp_config_get,
	.get_properties = dai_ti_mcasp_get_properties,
	.trigger        = dai_ti_mcasp_trigger,
};

#define DAI_TI_MCASP_DEFINE(inst)						\
static const struct dai_ti_mcasp_cfg dai_ti_mcasp_cfg_##inst = {	\
	.base          = DT_INST_REG_ADDR_BY_NAME(inst, mpu),		\
	.dat_base      = DT_INST_REG_ADDR_BY_NAME(inst, dat),		\
	.tx_dma_thread = DT_INST_DMAS_CELL_BY_NAME(inst, tx, thread_id), \
	.rx_dma_thread = DT_INST_DMAS_CELL_BY_NAME(inst, rx, thread_id), \
	.clk_dev       = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),	\
	.clk.dev_id    = DT_INST_CLOCKS_CELL(inst, devid),		\
	.clk.clk_id    = DT_INST_CLOCKS_CELL(inst, clkid),		\
	.clk_parent    = DT_INST_PHA_BY_IDX(inst, assigned_clock_parents, 0, clkid), \
	.clk_rate      = DT_INST_PROP_BY_IDX(inst, assigned_clock_rates, 0), \
	.n_aclk        = DT_INST_PROP_LEN(inst, assigned_clocks),		\
	.aclk_id       = { MCASP_ACLK_CELL(inst, assigned_clocks, 0),		\
			   MCASP_ACLK_CELL(inst, assigned_clocks, 1),		\
			   MCASP_ACLK_CELL(inst, assigned_clocks, 2) },		\
	.aclk_parent   = { MCASP_ACLK_CELL(inst, assigned_clock_parents, 0),	\
			   MCASP_ACLK_CELL(inst, assigned_clock_parents, 1),	\
			   MCASP_ACLK_CELL(inst, assigned_clock_parents, 2) },	\
	.aclk_rate     = { MCASP_ACLK_RATE(inst, 0), MCASP_ACLK_RATE(inst, 1),	\
			   MCASP_ACLK_RATE(inst, 2) },				\
	.tdm_slots     = DT_INST_PROP_OR(inst, tdm_slots, 0),		\
	.tdm_slots_rx  = DT_INST_PROP_OR(inst, tdm_slots_rx, 0),	\
	.tx_num_evt    = DT_INST_PROP_OR(inst, tx_num_evt, 0),		\
	.rx_num_evt    = DT_INST_PROP_OR(inst, rx_num_evt, 0),		\
	.ser_dir       = DT_INST_PROP(inst, serial_dir),		\
	.n_ser         = DT_INST_PROP_LEN(inst, serial_dir),		\
	.dismod        = DT_INST_PROP_OR(inst, dismod, 2),		\
	.async_mode    = DT_INST_PROP(inst, ti_async_mode),		\
	.self_ord      = DT_DEP_ORD(DT_DRV_INST(inst)),			\
	.has_tx_irq    = DT_INST_IRQ_HAS_NAME(inst, tx),		\
	.has_rx_irq    = DT_INST_IRQ_HAS_NAME(inst, rx),		\
	.tx_irq        = COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, tx),	\
			    (DT_INST_IRQ_BY_NAME(inst, tx, irq)), (0)),	\
	.tx_irq_prio   = COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, tx),	\
			    (DT_INST_IRQ_BY_NAME(inst, tx, priority)), (0)), \
	.rx_irq        = COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, rx),	\
			    (DT_INST_IRQ_BY_NAME(inst, rx, irq)), (0)),	\
	.rx_irq_prio   = COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, rx),	\
			    (DT_INST_IRQ_BY_NAME(inst, rx, priority)), (0)), \
};									\
static struct dai_ti_mcasp_data dai_ti_mcasp_data_##inst;		\
PM_DEVICE_DT_INST_DEFINE(inst, dai_ti_mcasp_pm_action);		\
DEVICE_DT_INST_DEFINE(inst, dai_ti_mcasp_init, PM_DEVICE_DT_INST_GET(inst), \
		      &dai_ti_mcasp_data_##inst,			\
		      &dai_ti_mcasp_cfg_##inst, POST_KERNEL,		\
		      CONFIG_DAI_INIT_PRIORITY, &dai_ti_mcasp_api);

DT_INST_FOREACH_STATUS_OKAY(DAI_TI_MCASP_DEFINE)
