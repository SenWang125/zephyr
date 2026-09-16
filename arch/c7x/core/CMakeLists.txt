#  Copyright (c) 2026 Texas Instruments Incorporated
#  SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
    prep_c.c
    thread.c
    fatal.c
    cache.c
    irq_manage.c
    cpu_idle.c
    mmu.c
    mmu_tables_access.c
    reset.c
    switch.S
    exception.S
    mmu.S
)

zephyr_library_sources_ifdef(CONFIG_DEBUG_COREDUMP coredump.c)
zephyr_library_sources_ifdef(CONFIG_C7X_OPTIMIZED_STRING memfuncs.c)
zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)

zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
