/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 *  Texas Instruments C7000 Compiler (cl7x)
 *  Because cl7x has __TI_GNU_ATTRIBUTE_SUPPORT__ == 1 it accepts most
 *  GCC __attribute__((...)) forms EXCEPT:
 *    - __attribute__((naked))
 *    - __attribute__((optimize("...")))
 *    - __attribute__((fallthrough))
 *    - __attribute__((optnone))
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_CL7X_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_CL7X_H_

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_H_
#error Please do not include toolchain-specific headers directly, use <zephyr/toolchain.h> instead
#endif

/* cl7x runs no preprocessor over assembly, so this header only ever sees C and C++. */

#define TOOLCHAIN_HAS_PRAGMA_DIAG 0
#define TOOLCHAIN_HAS_C_GENERIC 1
#define TOOLCHAIN_HAS_C_AUTO_TYPE 0
#define TOOLCHAIN_HAS_ZLA 1
#define TOOLCHAIN_HAS_ALLOCA 0
#define TOOLCHAIN_HAS_CONSTEXPR_CLZ 0
#define TOOLCHAIN_HAS_STMT_EXPR 0
#define TOOLCHAIN_HAS_VARIABLE_ALIAS 0

#ifndef __ORDER_LITTLE_ENDIAN__
#define __ORDER_LITTLE_ENDIAN__ 1234
#endif
#ifndef __ORDER_BIG_ENDIAN__
#define __ORDER_BIG_ENDIAN__ 4321
#endif
#ifndef __BYTE_ORDER__
#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
#endif

#if defined(__cplusplus)
#define BUILD_ASSERT(EXPR, MSG...) static_assert(EXPR, "" MSG)
#else
#define BUILD_ASSERT(EXPR, MSG...) _Static_assert((EXPR), "" MSG)
#endif

#include <zephyr/toolchain/common.h>
#include <stdbool.h>
#include <c7x.h>

#define ALIAS_OF(of)    __attribute__((alias(#of)))

#define FUNC_ALIAS(real_func, new_alias, return_type) \
	return_type new_alias() __attribute__((alias(#real_func)))

#define CODE_UNREACHABLE  do { for (;;) { } } while (0)

#define FUNC_NORETURN   __attribute__((__noreturn__))

#define _NODATA_SECTION(segment)    __attribute__((section(#segment)))

#define UNALIGNED_GET(g)                            \
({                                                  \
	__typeof__(*(g)) __v;                       \
	__builtin_memcpy(&__v, (g), sizeof(__v));   \
	__v;                                        \
})

#define UNALIGNED_PUT(v, p)                         \
do {                                                \
	__typeof__(v) __v = (v);                    \
	__builtin_memcpy((p), &__v, sizeof(__v));   \
} while (0)

#define UNALIGNED_MEMBER_ADDR(_p, _member) ((__typeof__(_p->_member) *) \
		(((intptr_t)(_p)) + offsetof(__typeof__(*_p), _member)))

#define __GENERIC_SECTION(segment)  __attribute__((section(#segment)))
#define Z_GENERIC_SECTION(segment)  __GENERIC_SECTION(segment)

#define __GENERIC_DOT_SECTION(segment) \
	__attribute__((section("." #segment)))
#define Z_GENERIC_DOT_SECTION(segment) __GENERIC_DOT_SECTION(segment)

#define ___in_section(a, b, c) \
	__attribute__((section("." #a "." #b "." #c)))
#define __in_section(a, b, c) ___in_section(a, b, c)

#define ___in_section_unique(a, b) \
	__attribute__((section("." #a "." #b)))
#define __in_section_unique(seg) \
	___in_section_unique(seg, __COUNTER__)
#define __in_section_unique_named(seg, name) \
	___in_section_unique(seg, name)

#define __ramfunc

#ifndef __fallthrough
#define __fallthrough
#endif

#ifndef __packed
#define __packed        __attribute__((__packed__))
#endif

#ifndef __aligned
#define __aligned(x)    __attribute__((__aligned__(x)))
#endif

#ifndef __noinline
#define __noinline	__attribute__((noinline))
#endif

#define __may_alias     __attribute__((__may_alias__))

#ifndef __printf_like
#define __printf_like(f, a)  __attribute__((format(printf, f, a)))
#endif

#define __used          __attribute__((__used__))
#define __unused        __attribute__((__unused__))
#define __maybe_unused	__attribute__((__unused__))

#ifndef __deprecated
#define __deprecated    __attribute__((deprecated))
#endif

#ifndef __deprecated_version
#define __deprecated_version(version) \
	__attribute__((deprecated("since " version)))
#endif

#ifndef __attribute_const__
#define __attribute_const__  __attribute__((__const__))
#endif

#ifndef __must_check
#define __must_check    __attribute__((warn_unused_result))
#endif

#define ARG_UNUSED(x) ((void)(x))

#define likely(x)   (__builtin_expect(!!(x), 1) != 0L)
#define unlikely(x) (__builtin_expect(!!(x), 0) != 0L)
#define POPCOUNT(x)	__builtin_popcount(x)

#ifndef __no_optimization
#define __no_optimization
#endif

#ifndef __weak
#define __weak          __attribute__((__weak__))
#endif

#ifndef __attribute_nonnull
#define __attribute_nonnull(...) __attribute__((nonnull(__VA_ARGS__)))
#endif

#ifndef __cleanup
#define __cleanup(x)	__attribute__((cleanup(x)))
#endif

#define __WARN(msg)
#define __DEPRECATED_MACRO

#define TOOLCHAIN_DISABLE_WARNING(warning)
#define TOOLCHAIN_ENABLE_WARNING(warning)
#define TOOLCHAIN_DISABLE_GCC_WARNING(warning)
#define TOOLCHAIN_ENABLE_GCC_WARNING(warning)

#define TOOLCHAIN_WARNING_ADDRESS_OF_PACKED_MEMBER
#define TOOLCHAIN_WARNING_ARRAY_BOUNDS
#define TOOLCHAIN_WARNING_ATTRIBUTES
#define TOOLCHAIN_WARNING_CAST_QUAL
#define TOOLCHAIN_WARNING_DELETE_NON_VIRTUAL_DTOR
#define TOOLCHAIN_WARNING_EXTRA
#define TOOLCHAIN_WARNING_NONNULL
#define TOOLCHAIN_WARNING_POINTER_ARITH
#define TOOLCHAIN_WARNING_SHADOW
#define TOOLCHAIN_WARNING_STRINGOP_OVERREAD
#define TOOLCHAIN_WARNING_UNUSED_LABEL
#define TOOLCHAIN_WARNING_UNUSED_VARIABLE

#define __no_instrumentation__
#define __noubsan
#define FUNC_NO_STACK_PROTECTOR

#define GEN_OFFSET_EXTERN(name) extern const char name[]

#define GEN_ABS_SYM_BEGIN(name) \
	void name(void)         \
	{                       \
		(void)0;

#define GEN_ABS_SYM_END }

#define GEN_ABSOLUTE_SYM(name, value) \
	static const volatile char name[(value) + 1] __attribute__((used))

#define GEN_ABSOLUTE_SYM_KCONFIG(name, value) \
	__asm__("\t.global " #name "\n" #name "\t.set\t" #value)

#define compiler_barrier() __asm("  NOP")

/* libmetal's atomic shim fences every shared-memory access with this */
#define __sync_synchronize() __memory_fence(__MFENCE_ALL_COLORS)

#define __builtin_assume_aligned(p, a) (p)

#ifndef __INT8_C
#define __INT8_C(x)	x
#endif
#ifndef INT8_C
#define INT8_C(x)	__INT8_C(x)
#endif
#ifndef __UINT8_C
#define __UINT8_C(x)	x ## U
#endif
#ifndef UINT8_C
#define UINT8_C(x)	__UINT8_C(x)
#endif
#ifndef __INT16_C
#define __INT16_C(x)	x
#endif
#ifndef INT16_C
#define INT16_C(x)	__INT16_C(x)
#endif
#ifndef __UINT16_C
#define __UINT16_C(x)	x ## U
#endif
#ifndef UINT16_C
#define UINT16_C(x)	__UINT16_C(x)
#endif
#ifndef __INT32_C
#define __INT32_C(x)	x
#endif
#ifndef INT32_C
#define INT32_C(x)	__INT32_C(x)
#endif
#ifndef __UINT32_C
#define __UINT32_C(x)	x ## U
#endif
#ifndef UINT32_C
#define UINT32_C(x)	__UINT32_C(x)
#endif
#ifndef __INT64_C
#define __INT64_C(x)	x ## LL
#endif
#ifndef INT64_C
#define INT64_C(x)	__INT64_C(x)
#endif
#ifndef __UINT64_C
#define __UINT64_C(x)	x ## ULL
#endif
#ifndef UINT64_C
#define UINT64_C(x)	__UINT64_C(x)
#endif
#ifndef __INTMAX_C
#define __INTMAX_C(x)	x ## LL
#endif
#ifndef INTMAX_C
#define INTMAX_C(x)	__INTMAX_C(x)
#endif
#ifndef __UINTMAX_C
#define __UINTMAX_C(x)	x ## ULL
#endif
#ifndef UINTMAX_C
#define UINTMAX_C(x)	__UINTMAX_C(x)
#endif

#define Z_IS_POW2(x) (((x) != 0) && (((x) & ((x)-1)) == 0))

#define __Z_POW2_SMEAR(v, s) ((v) | ((v) >> (s)))
#define __Z_POW2_CEIL_CONST(x) \
	(__Z_POW2_SMEAR(__Z_POW2_SMEAR(__Z_POW2_SMEAR(__Z_POW2_SMEAR( \
	 __Z_POW2_SMEAR(__Z_POW2_SMEAR((x) - 1UL, 1), 2), 4), 8), 16), 32) + 1UL)
#define Z_POW2_CEIL(x) \
	((x) <= 2UL ? (x) : \
	 __builtin_constant_p(x) ? __Z_POW2_CEIL_CONST(x) : \
	 (1UL << (8 * sizeof(long) - __builtin_clzl((x) - 1))))

/*
 * cl7x has none of these builtins and emits a call to a symbol of that name, so
 * they are supplied on the bit-scan and population-count instructions.
 */
static inline int __cl7x_clz(unsigned int x)
{
	return (int)__leftmost_bit_detect_one((__uint)x);
}
#define __builtin_clz(x)  __cl7x_clz((unsigned int)(x))

static inline int __cl7x_ctz(unsigned int x)
{
	return (int)__leftmost_bit_detect_one((__uint)__bit_reverse((__uint)x));
}
#define __builtin_ctz(x)  __cl7x_ctz((unsigned int)(x))

static inline int __cl7x_clzll(unsigned long long x)
{
	return (int)__leftmost_bit_detect_one((__ulong)x);
}
#define __builtin_clzll(x)  __cl7x_clzll((unsigned long long)(x))

static inline int __cl7x_ctzll(unsigned long long x)
{
	return (int)__leftmost_bit_detect_one((__ulong)__bit_reverse((__ulong)x));
}
#define __builtin_ctzll(x)  __cl7x_ctzll((unsigned long long)(x))

static inline int __cl7x_ffs(int x)
{
	unsigned int u = (unsigned int)x;

	return 32 - (int)__leftmost_bit_detect_one((__uint)(u & (0U - u)));
}
#define __builtin_ffs(x)  __cl7x_ffs((int)(x))

#define __builtin_clzl(x)   __cl7x_clzll((unsigned long long)(x))
#define __builtin_ctzl(x)   __cl7x_ctzll((unsigned long long)(x))
#define __builtin_popcount(x)   ((int)__popcount((__uint)(x)))

static inline int __cl7x_popcountll(unsigned long long x)
{
	return (int)__popcount((__ulong)x);
}
#define __builtin_popcountll(x) __cl7x_popcountll((unsigned long long)(x))

static inline int __cl7x_umul64_overflow(unsigned long long a, unsigned long long b,
					 unsigned long long *r)
{
	int spare = (int)__leftmost_bit_detect_one((__ulong)a) +
		    (int)__leftmost_bit_detect_one((__ulong)b);
	unsigned long long half = (a >> 1) * b;

	*r = a * b;
	if (spare != 63) {
		return spare < 63;
	}
	return (half >> 63) != 0 || ((a & 1U) != 0 && *r < b);
}

static inline int __cl7x_umul32_overflow(unsigned int a, unsigned int b, unsigned int *r)
{
	unsigned long long p = (unsigned long long)a * b;

	*r = (unsigned int)p;
	return (p >> 32) != 0;
}

static inline int __cl7x_umul16_overflow(unsigned short a, unsigned short b, unsigned short *r)
{
	unsigned int p = (unsigned int)a * b;

	*r = (unsigned short)p;
	return (p >> 16) != 0;
}

#define __builtin_mul_overflow(a, b, r)						\
	((void)sizeof(char[((__typeof__(*(r)))-1 > 0 &&			\
			    (sizeof(*(r)) == 2 || sizeof(*(r)) == 4 ||		\
			     sizeof(*(r)) == 8)) ? 1 : -1]),			\
	 sizeof(*(r)) == 8 ? __cl7x_umul64_overflow((a), (b), (unsigned long long *)(r)) : \
	 sizeof(*(r)) == 4 ? __cl7x_umul32_overflow((a), (b), (unsigned int *)(r)) :	\
	 __cl7x_umul16_overflow((a), (b), (unsigned short *)(r)))

static inline unsigned short __cl7x_bswap16(unsigned short x)
{
	return (unsigned short)((x << 8) | (x >> 8));
}
#define __builtin_bswap16(x) __cl7x_bswap16((unsigned short)(x))

static inline unsigned int __cl7x_bswap32(unsigned int x)
{
	return ((unsigned int)__cl7x_bswap16((unsigned short)x) << 16) |
	       __cl7x_bswap16((unsigned short)(x >> 16));
}
#define __builtin_bswap32(x) __cl7x_bswap32((unsigned int)(x))

static inline unsigned long long __cl7x_bswap64(unsigned long long x)
{
	return ((unsigned long long)__cl7x_bswap32((unsigned int)x) << 32) |
	       __cl7x_bswap32((unsigned int)(x >> 32));
}
#define __builtin_bswap64(x) __cl7x_bswap64((unsigned long long)(x))

/* cl7x answers __has_builtin 0 for every builtin except __builtin_expect.
 */
#undef HAS_BUILTIN
#define HAS_BUILTIN(x)			HAS_BUILTIN_##x
#define HAS_BUILTIN___builtin_bswap16	1
#define HAS_BUILTIN___builtin_bswap32	1
#define HAS_BUILTIN___builtin_bswap64	1
#define HAS_BUILTIN___builtin_clz	1
#define HAS_BUILTIN___builtin_clzl	1
#define HAS_BUILTIN___builtin_clzll	1
#define HAS_BUILTIN___builtin_ctz	1
#define HAS_BUILTIN___builtin_ctzl	1
#define HAS_BUILTIN___builtin_ctzll	1
#define HAS_BUILTIN___builtin_ffs	1
#define HAS_BUILTIN___builtin_popcount	1
#define HAS_BUILTIN___builtin_popcountll	1
#define HAS_BUILTIN___builtin_mul_overflow	1

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_CL7X_H_ */
