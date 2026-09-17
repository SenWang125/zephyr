# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/cmake/linker_script/c7x/linker.cmake CACHE INTERNAL "")

zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)
