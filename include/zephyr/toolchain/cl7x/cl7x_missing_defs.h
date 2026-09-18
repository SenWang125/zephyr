/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_CL7X_CL7X_MISSING_DEFS_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_CL7X_CL7X_MISSING_DEFS_H_

#include <stdbool.h>

#define __SIZEOF_POINTER__ 8

typedef signed char   __cl7x_int8;
#define __INT8_TYPE__         __cl7x_int8
typedef short         __cl7x_int16;
#define __INT16_TYPE__        __cl7x_int16
typedef int           __cl7x_int32;
#define __INT32_TYPE__        __cl7x_int32
typedef long long     __cl7x_int64;
#define __INT64_TYPE__        __cl7x_int64

typedef unsigned char  __cl7x_uint8;
#define __UINT8_TYPE__        __cl7x_uint8
typedef unsigned short __cl7x_uint16;
#define __UINT16_TYPE__       __cl7x_uint16
typedef unsigned int   __cl7x_uint32;
#define __UINT32_TYPE__       __cl7x_uint32
typedef unsigned long long __cl7x_uint64;
#define __UINT64_TYPE__       __cl7x_uint64

#define __INT_LEAST8_TYPE__   __cl7x_int8
#define __INT_LEAST16_TYPE__  __cl7x_int16
#define __INT_LEAST32_TYPE__  __cl7x_int32
#define __INT_LEAST64_TYPE__  __cl7x_int64
#define __UINT_LEAST8_TYPE__  __cl7x_uint8
#define __UINT_LEAST16_TYPE__ __cl7x_uint16
#define __UINT_LEAST32_TYPE__ __cl7x_uint32
#define __UINT_LEAST64_TYPE__ __cl7x_uint64

#define __INT_FAST8_TYPE__    __cl7x_int32
#define __INT_FAST16_TYPE__   __cl7x_int32
#define __INT_FAST32_TYPE__   __cl7x_int32
#define __INT_FAST64_TYPE__   __cl7x_int64
#define __UINT_FAST8_TYPE__   __cl7x_uint32
#define __UINT_FAST16_TYPE__  __cl7x_uint32
#define __UINT_FAST32_TYPE__  __cl7x_uint32
#define __UINT_FAST64_TYPE__  __cl7x_uint64

typedef long           __cl7x_intptr;
#define __INTPTR_TYPE__       __cl7x_intptr
typedef unsigned long  __cl7x_uintptr;
#define __UINTPTR_TYPE__      __cl7x_uintptr

#define __INTMAX_TYPE__       __cl7x_int64
#define __UINTMAX_TYPE__      __cl7x_uint64

typedef unsigned int   __cl7x_wchar;
#define __WCHAR_TYPE__        __cl7x_wchar

typedef unsigned long  __cl7x_size;
#define __SIZE_TYPE__         __cl7x_size
typedef long           __cl7x_ptrdiff;
#define __PTRDIFF_TYPE__      __cl7x_ptrdiff

#define __va_list_defined
typedef __builtin_va_list __va_list;

#define __UINT8_MAX__    (255U)
#define __UINT16_MAX__   (65535U)
#define __UINT32_MAX__   (4294967295U)
#define __UINT64_MAX__   (18446744073709551615ULL)
#define __INT8_MAX__     (127)
#define __INT16_MAX__    (32767)
#define __INT32_MAX__    (2147483647)
#define __INT64_MAX__    (9223372036854775807LL)

/* stdint.h's least/fast widths resolve through these; the compiler omits them. */
#define __INT_LEAST8_MAX__   __INT8_MAX__
#define __INT_LEAST16_MAX__  __INT16_MAX__
#define __INT_LEAST32_MAX__  __INT32_MAX__
#define __INT_LEAST64_MAX__  __INT64_MAX__
#define __UINT_LEAST8_MAX__  __UINT8_MAX__
#define __UINT_LEAST16_MAX__ __UINT16_MAX__
#define __UINT_LEAST32_MAX__ __UINT32_MAX__
#define __UINT_LEAST64_MAX__ __UINT64_MAX__
#define __INT_FAST8_MAX__    __INT32_MAX__
#define __INT_FAST16_MAX__   __INT32_MAX__
#define __INT_FAST32_MAX__   __INT32_MAX__
#define __INT_FAST64_MAX__   __INT64_MAX__
#define __UINT_FAST64_MAX__  __UINT64_MAX__
#define __INTMAX_MAX__       __INT64_MAX__
#define __UINTMAX_MAX__      __UINT64_MAX__
#define __SIZEOF_INTMAX__    8
#define __SIZEOF_UINTMAX__   8
#define __UINT_FAST8_MAX__   __UINT32_MAX__
#define __UINT_FAST16_MAX__  __UINT32_MAX__
#define __UINT_FAST32_MAX__  __UINT32_MAX__
#define __UINTPTR_MAX__      __UINT64_MAX__
#define __INTPTR_MAX__       __INT64_MAX__
#define __SIZE_MAX__         __UINT64_MAX__
#define __PTRDIFF_MAX__      __INT64_MAX__

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_CL7X_CL7X_MISSING_DEFS_H_ */
