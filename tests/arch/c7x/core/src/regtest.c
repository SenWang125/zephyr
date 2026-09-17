/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Per-register preservation test for the C7x interrupt path.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <string.h>

#define OFF_A     0x000
#define OFF_D     0x080
#define OFF_P     0x180
#define OFF_VB    0xA00
#define OFF_VBL   0x400
#define OFF_VBM   0x500
#define OFF_CUCR  0x600
#define BUF_SZ    0x1200

#define OFF_SW    0x800
#define OFF_AL    0xC00
#define OFF_AM    0xC80
#define OFF_CTL   0xD00
#define OFF_CTL2  0xD20

extern void c7x_regtest_isr(const void *pat, void *out, volatile uint32_t *flag);
extern void c7x_regtest_dfile(const void *pat, void *out, volatile uint32_t *flag);
extern void *c7x_regtest_switch(const void *pat, void *out, void (*fn)(void));

static K_THREAD_STACK_DEFINE(rt_peer_stack, 2048);
static struct k_thread rt_peer;
static volatile int rt_peer_run;

/* k_yield only switches if another thread of equal priority is ready */
static void rt_peer_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	while (rt_peer_run) {
		k_yield();
	}
}

static void rt_noop(void) { }

static uint8_t rt_pat[BUF_SZ] __aligned(64);
static uint8_t rt_ref[BUF_SZ] __aligned(64);
static uint8_t rt_out[BUF_SZ] __aligned(64);
static volatile uint32_t rt_flag;
static volatile uint32_t rt_irqs;

extern char c7x_isr_stack[CONFIG_ISR_STACK_SIZE];
static volatile uintptr_t rt_isr_sp;

static void rt_tick(struct k_timer *t)
{
	volatile int probe;

	ARG_UNUSED(t);
	rt_isr_sp = (uintptr_t)&probe;
	rt_irqs++;
	if (rt_irqs >= 3U) {
		rt_flag = 1U;
	}
}

static K_TIMER_DEFINE(rt_timer, rt_tick, NULL);

static uint64_t rt_rand(uint64_t *s)
{
	*s ^= *s << 13;
	*s ^= *s >> 7;
	*s ^= *s << 17;
	return *s;
}

static void rt_fill(void)
{
	uint64_t s = 0xC7C7A5A500000001ULL;

	for (unsigned int i = 0; i < BUF_SZ / 8U; i++) {
		((uint64_t *)rt_pat)[i] = rt_rand(&s);
	}
}

/* Same fill/dump with interrupts locked. The baseline absorbs registers that are
 * narrower than 64 bits (P), which never read back what was written.
 */
static void rt_baseline(void (*fn)(const void *, void *, volatile uint32_t *))
{
	unsigned int k;

	memset(rt_ref, 0, sizeof(rt_ref));
	rt_flag = 1U;
	k = irq_lock();
	fn(rt_pat, rt_ref, &rt_flag);
	irq_unlock(k);
}

static int rt_measure(void (*fn)(const void *, void *, volatile uint32_t *))
{
	memset(rt_out, 0, sizeof(rt_out));
	rt_flag = 0U;
	rt_irqs = 0U;
	k_timer_start(&rt_timer, K_MSEC(1), K_MSEC(1));
	fn(rt_pat, rt_out, &rt_flag);
	k_timer_stop(&rt_timer);
	return (int)rt_irqs;
}

/* skip is a bitmask of indices this pass does not dump (its own harness) */
static unsigned int rt_class(const char *name, unsigned int off, unsigned int n,
		unsigned int stride, uint32_t skip)
{
	unsigned int bad = 0;

	for (unsigned int i = 0; i < n; i++) {
		if (skip & BIT(i)) {
			continue;
		}
		if (memcmp(rt_ref + off + i * stride, rt_out + off + i * stride,
			   stride) != 0) {
			TC_PRINT("register %s%u changed across an interrupt\n", name, i);
			bad++;
		}
	}
	return bad;
}

ZTEST(c7x_core, test_isr_preserves_registers)
{
	unsigned int bad = 0;
	int n;

	/* pass 1 harness is A5 and D0-D2, so it cannot report on them */
	rt_fill();
	rt_baseline(c7x_regtest_isr);
	n = rt_measure(c7x_regtest_isr);
	bad += rt_class("A", OFF_A, 16, 8, BIT(5));
	bad += rt_class("D", OFF_D, 15, 8, BIT(0) | BIT(1) | BIT(2));
	bad += rt_class("P", OFF_P, 8, 8, 0);
	bad += rt_class("VB", OFF_VB, 16, 32, 0);
	bad += rt_class("VBL", OFF_VBL, 8, 32, 0);
	bad += rt_class("VBM", OFF_VBM, 8, 32, 0);
	bad += rt_class("CUCR", OFF_CUCR, 8, 32, 0);
	bad += rt_class("AL", OFF_AL, 16, 8, 0);
	bad += rt_class("AM", OFF_AM, 16, 8, 0);

	/* control state pre vs post within the same run */
	for (unsigned int i = 0; i < 4; i++) {
		static const char *const ctl[] = {"FPCR", "FSR", "GPLY", "GFPGFR"};

		if (((uint64_t *)(rt_out + OFF_CTL))[i] != ((uint64_t *)(rt_out + OFF_CTL2))[i]) {
			TC_PRINT("control register %s changed across an interrupt\n", ctl[i]);
			bad++;
		}
	}

	zassert_true(n >= 3, "only %d interrupts were taken", n);
	zassert_equal(bad, 0U, "%u registers were not preserved", bad);
}

ZTEST(c7x_core, test_isr_preserves_d_file_and_a5)
{
	unsigned int bad = 0;
	int n;

	/* pass 2 covers exactly what pass 1 could not. D0-D14 and A5 */
	rt_fill();
	rt_baseline(c7x_regtest_dfile);
	n = rt_measure(c7x_regtest_dfile);
	bad += rt_class("p2D", OFF_D, 15, 8, 0);
	bad += rt_class("p2A", OFF_A, 6, 8, BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4));

	zassert_true(n >= 3, "only %d interrupts were taken", n);
	zassert_equal(bad, 0U, "%u registers were not preserved", bad);
}

ZTEST(c7x_core, test_isr_runs_on_isr_stack)
{
	rt_fill();
	rt_isr_sp = 0U;
	zassert_true(rt_measure(c7x_regtest_isr) >= 3, "no interrupt was taken");

	/* handlers must run on the dedicated ISR stack, not the thread stack */
	zassert_true(rt_isr_sp >= (uintptr_t)c7x_isr_stack &&
		     rt_isr_sp < (uintptr_t)c7x_isr_stack + CONFIG_ISR_STACK_SIZE,
		     "handler sp 0x%lx outside the ISR stack", (unsigned long)rt_isr_sp);
}

ZTEST(c7x_core, test_switch_preserves_callee_saved)
{
	unsigned int bad = 0;

	/* context switch. Only the callee-saved set, which is all TaskSupport_swap
	 * preserves too. A8-A15 plus the 64-bit B14/B15
	 */
	rt_fill();
	rt_peer_run = 1;
	k_thread_create(&rt_peer, rt_peer_stack, K_THREAD_STACK_SIZEOF(rt_peer_stack),
			rt_peer_fn, NULL, NULL, NULL,
			k_thread_priority_get(k_current_get()), 0, K_NO_WAIT);
	memset(rt_ref, 0, sizeof(rt_ref));
	void *g1 = c7x_regtest_switch(rt_pat, rt_ref, rt_noop);

	memset(rt_out, 0, sizeof(rt_out));
	void *g2 = c7x_regtest_switch(rt_pat, rt_out, k_yield);

	rt_peer_run = 0;
	k_thread_join(&rt_peer, K_FOREVER);

	bad += rt_class("swA", OFF_A, 16, 8, 0xFFU);          /* A8-A15 only */
	bad += rt_class("swB", OFF_SW + 0x40, 2, 8, 0);       /* B14, B15 */

	zassert_equal_ptr(g1, rt_ref, "harness returned %p, not its out buffer", g1);
	zassert_equal_ptr(g2, rt_out, "harness returned %p after a switch", g2);
	zassert_equal(bad, 0U, "%u callee-saved registers were not preserved", bad);
}
