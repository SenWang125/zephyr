#  Copyright (c) 2026 Texas Instruments Incorporated
#  SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/compiler/compiler_flags_template.cmake)

set_compiler_property(PROPERTY cstd "--")

set_compiler_property(PROPERTY imacros "--preinclude")

set_compiler_property(PROPERTY nostdinc "")
set_property(TARGET compiler-cpp PROPERTY nostdincxx "")

set_compiler_property(PROPERTY warnings_as_errors "--emit_warnings_as_errors")

set_compiler_property(PROPERTY no_optimization      "--opt_level=off")
set_compiler_property(PROPERTY optimization_debug   "--opt_level=off")
set_compiler_property(PROPERTY optimization_speed   "--opt_level=3")
set_compiler_property(PROPERTY optimization_size    "--opt_level=2" "--opt_for_speed=0")

set_compiler_property(PROPERTY debug "--symdebug:dwarf")

# CONFIG_SIZE_OPTIMIZATIONS_AGGRESSIVE otherwise leaves the -O flag empty and
# cl7x falls back to --opt_level=0, i.e. optimisation off.
set_compiler_property(PROPERTY optimization_size_aggressive "--opt_level=2" "--opt_for_speed=0")
set_compiler_property(PROPERTY optimization_fast "--opt_level=3" "--opt_for_speed=5")

# gen_kobject_list.py cannot resolve per-function subsections; -mo is global here.
set_compiler_property(PROPERTY no_function_sections "--gen_func_subsections=off")
set_compiler_property(PROPERTY no_data_sections "--gen_data_subsections=off")

# cl7x defaults to --common=on, so tentative definitions become ELF commons and
# the generated .cmd file cannot place them by name.
set_compiler_property(PROPERTY no_common "--common=off")

set_compiler_property(PROPERTY warning_error_implicit_int "--diag_error=262")
set_compiler_property(PROPERTY include_file "--preinclude")

# set_compiler_property only reaches compiler/compiler-cpp, so asm needs its own.
set_property(TARGET asm PROPERTY warnings_as_errors "--emit_warnings_as_errors")

set_compiler_property(PROPERTY linker_script "")

set_property(TARGET compiler-cpp PROPERTY dialect_cpp14 "")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp17 "")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp2a "")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp20 "")
set_property(TARGET compiler-cpp PROPERTY no_exceptions "")
set_property(TARGET compiler-cpp PROPERTY no_rtti "")
set_property(TARGET compiler-cpp PROPERTY no_threadsafe_statics "")

set_property(TARGET asm PROPERTY imacros "--preinclude")

# cl7x exits 0 on an invalid option, so unsupported flags cannot be probed and are listed here.
set(_cl7x_gcc_only_options
  --param=min-pagesize=0
  -Wno-vla
  -fno-asynchronous-unwind-tables
  -fno-defer-pop
  -fno-omit-frame-pointer
  -fno-reorder-functions
  -fdata-sections
  -fno-pic
  -fno-common
  -fno-builtin
  -fno-strict-aliasing
  -fno-printf-return-value
  -fno-freestanding
)
# cl7x reads -ff as its listing-file-directory option and silently swallows
# "unction-sections", so the probe passes and every compile then emits E1000.
list(APPEND _cl7x_gcc_only_options -ffunction-sections)

list(APPEND C_EXCLUDED_OPTIONS   ${_cl7x_gcc_only_options})
list(APPEND CXX_EXCLUDED_OPTIONS ${_cl7x_gcc_only_options})
list(APPEND ASM_EXCLUDED_OPTIONS ${_cl7x_gcc_only_options})

# TI diagnostics are on by default; each suppression is a diagnostic left unfixed, not harmless.
set(CL7X_SUPPRESSED_WARNINGS)

list(APPEND CL7X_SUPPRESSED_WARNINGS
     --diag_suppress=163  # unrecognized #pragma
     --diag_suppress=1173 # unknown attribute
     --diag_suppress=1986 # array of elements containing a flexible array member
     --diag_suppress=193  # type qualifier is meaningless on cast type
     --diag_suppress=383  # extra ";" ignored
     --diag_suppress=112  # statement is unreachable
     --diag_suppress=129  # loop is not reachable
     --diag_suppress=187  # dynamic initialization in unreachable code
     --diag_suppress=552  # variable was set but never used
     # 548: goto past a later initializer; legal C goto-cleanup idiom, TI cannot
     # separate benign uses.
     --diag_suppress=548
     # 190: integer expression assigned to an enum object; legal C, a C++ error, GCC does not warn
     --diag_suppress=190
)

# Left ON deliberately -- each can indicate a real defect, and none is a
# consequence of cl7x differing from GCC: 69/70 integer conversion sign change
# and truncation, 770 pointer to smaller integer, 183 format string mismatch,
# 161 incompatible declaration, 145/169/515 pointer type mismatch, 188
# pointless unsigned comparison, 1291 field shadowed.

# cl7x has no stack-protector option, so the security_canaries properties stay
# empty and the build would report protection it does not emit.
if(CONFIG_REQUIRES_STACK_CANARIES)
  message(FATAL_ERROR "CONFIG_STACK_CANARIES is not supported: cl7x cannot emit stack canary instrumentation.")
endif()

set_compiler_property(PROPERTY warning_base
  --display_error_number
  ${CL7X_SUPPRESSED_WARNINGS}
)

# TI assembly is not run through the C preprocessor, so CONFIG_ symbols reach
# .S files as --asm_define rather than through imacros.
if(NOT DEFINED AUTOCONF_H OR NOT EXISTS "${AUTOCONF_H}")
  message(FATAL_ERROR
    "cl7x: AUTOCONF_H='${AUTOCONF_H}' is unset or missing. No CONFIG_ symbol would "
    "reach a .S file and every conditional-assembly test there would take its false "
    "arm without a diagnostic. The kconfig module must run before this one.")
endif()
set(_cl7x_asm_defines "")
file(STRINGS "${AUTOCONF_H}" _cl7x_autoconf_lines REGEX "^#define CONFIG_")
foreach(_line ${_cl7x_autoconf_lines})
  if(_line MATCHES "^#define (CONFIG_[A-Za-z0-9_]+) (.*)$")
    set(_name "${CMAKE_MATCH_1}")
    set(_value "${CMAKE_MATCH_2}")
    if(_value STREQUAL "1")
      list(APPEND _cl7x_asm_defines "--asm_define=${_name}")
    elseif(NOT _value MATCHES "^\"")
      list(APPEND _cl7x_asm_defines "--asm_define=${_name}=${_value}")
    endif()
  endif()
endforeach()
list(LENGTH _cl7x_asm_defines _cl7x_asm_define_count)
if(_cl7x_asm_define_count EQUAL 0)
  message(FATAL_ERROR "cl7x: ${AUTOCONF_H} yielded no CONFIG_ symbol to --asm_define")
endif()
message(STATUS "cl7x: ${_cl7x_asm_define_count} CONFIG_ symbols to --asm_define")
list(JOIN _cl7x_asm_defines " " _cl7x_asm_defines_str)
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${_cl7x_asm_defines_str}")
