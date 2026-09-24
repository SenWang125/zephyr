# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/cmake/linker_script/c7x/linker.cmake CACHE INTERNAL "")

zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_TI_K3_C7X_FATAL_SHUTDOWN_ACK fatal.c)
# The generic table in subsys/ipc/open-amp carries a trace entry only with
# CONFIG_RAM_CONSOLE. Firmware that keeps its own console uses this one.
if(CONFIG_OPENAMP_RSC_TABLE AND NOT CONFIG_RAM_CONSOLE)
  zephyr_library_sources(rsc_table.c)
endif()
zephyr_library_sources(mmu_regions.c)

# The resource table must keep real call boundaries: see check_inlining_policy.py.
zephyr_library_compile_options(--disable_inlining)
