# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/cmake/linker_script/c7x/linker.cmake CACHE INTERNAL "")

zephyr_include_directories(.)

zephyr_library_sources(soc.c)

zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)
zephyr_library_sources_ifdef(CONFIG_C7X_SOC_SYSTEM_HALT_HOOK fatal.c)
zephyr_library_sources(mmu_regions.c)
