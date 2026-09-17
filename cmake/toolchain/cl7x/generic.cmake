# Copyright (c) 2025 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

zephyr_get(CL7X_TOOLCHAIN_PATH)

# Also accept CGT_TI_C7000_PATH, the variable TI's tools export, as arcmwdt
# accepts METAWARE_ROOT.
set(CGT_TI_C7000_PATH $ENV{CGT_TI_C7000_PATH})
if(NOT DEFINED CL7X_TOOLCHAIN_PATH AND DEFINED CGT_TI_C7000_PATH)
  message(STATUS "CL7X_TOOLCHAIN_PATH is not set, using CGT_TI_C7000_PATH: '${CGT_TI_C7000_PATH}'")
  set(CL7X_TOOLCHAIN_PATH ${CGT_TI_C7000_PATH})
endif()
assert(CL7X_TOOLCHAIN_PATH "CL7X_TOOLCHAIN_PATH is not set")

if(NOT EXISTS ${CL7X_TOOLCHAIN_PATH})
  message(FATAL_ERROR "Nothing found at CL7X_TOOLCHAIN_PATH: '${CL7X_TOOLCHAIN_PATH}'")
endif()

if(NOT EXISTS "${CL7X_TOOLCHAIN_PATH}/bin/cl7x")
  message(FATAL_ERROR "No cl7x compiler at CL7X_TOOLCHAIN_PATH: '${CL7X_TOOLCHAIN_PATH}'")
endif()

set(CL7X_MINIMUM_REQUIRED_VERSION 5.0.1)

message(STATUS "Found TI C7x toolchain at: ${CL7X_TOOLCHAIN_PATH}")

find_package(Zephyr-sdk REQUIRED)
message(STATUS "cl7x: Using Zephyr SDK at ${ZEPHYR_SDK_INSTALL_DIR} for DTS/bintools")

set(ZEPHYR_TOOLCHAIN_VARIANT cl7x)

set(COMPILER  cl7x)
set(LINKER    cl7x)
set(BINTOOLS  cl7x)

set(TOOLCHAIN_HOME "${CL7X_TOOLCHAIN_PATH}")
set(CROSS_COMPILE  "${CL7X_TOOLCHAIN_PATH}/bin/")

set(TOOLCHAIN_HAS_NEWLIB   OFF CACHE BOOL "cl7x does not support newlib")
set(TOOLCHAIN_HAS_PICOLIBC OFF CACHE BOOL "cl7x does not support picolibc")

set(CMAKE_C_BYTE_ORDER   LITTLE_ENDIAN)
set(CMAKE_CXX_BYTE_ORDER LITTLE_ENDIAN)

