# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED TOOLCHAIN_HOME)
  if(DEFINED ENV{CL7X_TOOLCHAIN_PATH})
    set(TOOLCHAIN_HOME $ENV{CL7X_TOOLCHAIN_PATH})
  elseif(DEFINED CL7X_TOOLCHAIN_PATH)
    set(TOOLCHAIN_HOME ${CL7X_TOOLCHAIN_PATH})
  else()
    message(FATAL_ERROR "cl7x: TOOLCHAIN_HOME not set. "
            "Set CL7X_TOOLCHAIN_PATH to the cl7x toolchain directory.")
  endif()
endif()

set(CMAKE_C_COMPILER   "${TOOLCHAIN_HOME}/bin/cl7x" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_HOME}/bin/cl7x" CACHE FILEPATH "" FORCE)
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_HOME}/bin/cl7x" CACHE FILEPATH "" FORCE)

set(CMAKE_C_OUTPUT_EXTENSION   ".obj" CACHE STRING "" FORCE)
set(CMAKE_ASM_OUTPUT_EXTENSION ".obj")

set(CMAKE_DEPFILE_FLAGS_C "--preproc_with_compile --preproc_dependency=<DEP_FILE>")
set(CMAKE_DEPFILE_FLAGS_CXX "--preproc_with_compile --preproc_dependency=<DEP_FILE>")
set(CMAKE_C_DEPFILE_FORMAT gcc)
set(CMAKE_CXX_DEPFILE_FORMAT gcc)
set(CMAKE_CXX_OUTPUT_EXTENSION ".obj" CACHE STRING "" FORCE)

set(CMAKE_C_COMPILER_ID   "TI_cl7x" CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER_ID "TI_cl7x" CACHE STRING "" FORCE)
set(CMAKE_ASM_COMPILER_ID "TI" CACHE STRING "" FORCE)

set(CMAKE_ASM_COMPILER_FORCED TRUE)

set(NOSTDINC "")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY CACHE STRING "" FORCE)

function(compiler_set_linker_properties)
  message(STATUS "cl7x: skipping GCC-specific runtime library detection")
endfunction()

if(CONFIG_CPU_C7504)
  set(CL7X_SILICON_VERSION 7504)
else()
  message(FATAL_ERROR "cl7x: the SoC selects no C7x CPU symbol, so there is no --silicon_version to build for")
endif()
set(CL7X_ISA_FLAG "-mv${CL7X_SILICON_VERSION}")

# Global. TOOLCHAIN_C_FLAGS reaches zephyr_interface only, and the module
# libraries are plain add_library targets outside it.
set(_CL7X_COMMON_FLAGS
  "${CL7X_ISA_FLAG}"
  "--abi=eabi"
  "--preinclude=${ZEPHYR_BASE}/include/zephyr/toolchain/cl7x/cl7x_missing_defs.h"
  "-I${TOOLCHAIN_HOME}/include"
  "-I${ZEPHYR_BASE}/include/zephyr/toolchain/cl7x"
  "-mo"
)
list(JOIN _CL7X_COMMON_FLAGS " " _CL7X_COMMON_FLAGS_STR)
set(CMAKE_C_FLAGS_INIT   "${_CL7X_COMMON_FLAGS_STR}")
set(CMAKE_CXX_FLAGS_INIT "${_CL7X_COMMON_FLAGS_STR} --c++14")
# .cdecls parses C headers with the assembler's command line, so it needs the C preinclude and path.
set(CMAKE_ASM_FLAGS_INIT
  "${CL7X_ISA_FLAG} --preinclude=${ZEPHYR_BASE}/include/zephyr/toolchain/cl7x/cl7x_missing_defs.h -I${TOOLCHAIN_HOME}/include")

set(CMAKE_C_COMPILE_OBJECT
  "<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> <FLAGS> --output_file=<OBJECT> -c <SOURCE>")
set(CMAKE_CXX_COMPILE_OBJECT
  "<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> --output_file=<OBJECT> -c <SOURCE>")
set(CMAKE_ASM_COMPILE_OBJECT
  "<CMAKE_ASM_COMPILER> <DEFINES> <INCLUDES> <FLAGS> --output_file=<OBJECT> -c <SOURCE>")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
  CMAKE_C_COMPILE_OBJECT CMAKE_CXX_COMPILE_OBJECT CMAKE_ASM_COMPILE_OBJECT)
