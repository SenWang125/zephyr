# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(c7x_core)

set(REGTEST_ASM ${CMAKE_CURRENT_BINARY_DIR}/regtest.S)
add_custom_command(
	OUTPUT ${REGTEST_ASM}
	COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen_regtest_asm.py ${REGTEST_ASM}
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gen_regtest_asm.py
)

target_sources(app PRIVATE src/main.c src/regtest.c src/cache_test.c ${REGTEST_ASM})

# The board has no UART. Printk reaches the host through the c7x-sof trace ring,
# which the resource table exposes as remoteproc's trace0.
if(ZEPHYR_C7X_SOF_MODULE_DIR)
  target_sources(app PRIVATE ${ZEPHYR_C7X_SOF_MODULE_DIR}/src/lib/trace_printk.c)
endif()
