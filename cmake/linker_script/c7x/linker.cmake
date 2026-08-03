# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0
#
# Rendered into a TI .cmd by cmake/linker/cl7x/cmd_script.cmake.
#
# The firmware owns the zephyr,sram window and nothing else. Every region is
# inside that window.

zephyr_linker(ENTRY ${CONFIG_KERNEL_ENTRY})

set(COMMON_ZEPHYR_LINKER_DIR ${ZEPHYR_BASE}/cmake/linker_script/common)

zephyr_linker_include_var(VAR C7X_CMD_DIAG_SUPPRESS VALUE "10068 10063")

# cl7x drops these without a reference; KEEP inputs are retained by the generator.
zephyr_linker_include_var(VAR C7X_CMD_RETAIN VALUE
  "*(.__static_thread_data.static*) \
*(.c7x_ecsp_area) \
*(.c7x_tcsp_area)")

# TI attributes the generic model has no equivalent for, keyed by section
# name with non-alphanumerics folded to '_'.
zephyr_linker_include_var(VAR C7X_TI_ATTR_bss_c7x_diag VALUE "RUN_START(__c7x_diag_start) RUN_END(__c7x_diag_end)")
zephyr_linker_include_var(VAR C7X_TI_ATTR_bss
  VALUE "RUN_START(__bss_start) RUN_END(__bss_end)")

dt_comp_path(c7x_memory_regions COMPATIBLE "zephyr,memory-region")
foreach(path IN LISTS c7x_memory_regions)
  zephyr_linker_dts_memory(PATH ${path})
endforeach()

if(CONFIG_MMU)
  # kernel/mmu.c pins the page frames from z_mapped_start to z_mapped_end, the image.
  dt_chosen(c7x_sram_path PROPERTY "zephyr,sram")
  dt_reg_addr(c7x_sram_base PATH ${c7x_sram_path})
  zephyr_linker_symbol(SYMBOL z_mapped_start EXPR "${c7x_sram_base}")
  zephyr_linker_include_var(VAR C7X_TI_MEM_ATTR_SRAM VALUE "LAST(__c7x_sram_last)")
  zephyr_linker_symbol(SYMBOL z_mapped_end EXPR
    "((@__c7x_sram_last@ + ${CONFIG_MMU_PAGE_SIZE} - 1) & ~(${CONFIG_MMU_PAGE_SIZE} - 1))")
endif()

# Scratch for the pre-pass .intList declared by arch/common; discarded before
# the final link.
zephyr_linker_memory(NAME IDT_LIST FLAGS wx START 0xFFFF8000 SIZE 2K)

zephyr_linker_group(NAME IPC_RSC_REGION LMA IPC_RSC VMA IPC_RSC)
zephyr_linker_group(NAME DIAG_REGION    LMA DIAG    VMA DIAG)
zephyr_linker_group(NAME VECTORS_REGION LMA VECTORS VMA VECTORS)

# Group order is emission order, and TI allocates a region in listed order.
zephyr_linker_group(NAME ROM_REGION    LMA SRAM VMA SRAM)
zephyr_linker_group(NAME TEXT_REGION   GROUP ROM_REGION)
zephyr_linker_group(NAME RODATA_REGION GROUP ROM_REGION)
zephyr_linker_group(NAME RAM_REGION    LMA SRAM VMA SRAM)
zephyr_linker_group(NAME DATA_REGION   GROUP RAM_REGION)
zephyr_linker_group(NAME NOINIT_REGION GROUP RAM_REGION)

zephyr_linker_section(NAME .resource_table GROUP IPC_RSC_REGION NOINPUT)
zephyr_linker_section_configure(SECTION .resource_table INPUT ".resource_table"
                                SYMBOLS __RESOURCE_TABLE)

zephyr_linker_section(NAME .exc_vector_table GROUP VECTORS_REGION ALIGN 0x400000)

zephyr_linker_section(NAME .text:_c_int00_secure GROUP TEXT_REGION NOINPUT ALIGN 0x200000)
zephyr_linker_section_configure(SECTION .text:_c_int00_secure INPUT ".text:_c_int00_secure")

# ALIGN(0x200000) on the entry and .text keeps the layout insensitive to size.
zephyr_linker_section(NAME .text GROUP TEXT_REGION NOINPUT ALIGN 0x200000)
zephyr_linker_section_configure(SECTION .text
                                INPUT ".text;.text.*")

# arch/common owns .intList for the earlier passes, where it goes to IDT_LIST.
zephyr_linker_section(NAME .intList GROUP TEXT_REGION NOINPUT PASS LINKER_ZEPHYR_FINAL)

zephyr_linker_section(NAME .gnu.linkonce.irq_vector_table GROUP TEXT_REGION NOINPUT ALIGN 16)
zephyr_linker_section(NAME .gnu.linkonce.sw_isr_table GROUP TEXT_REGION NOINPUT ALIGN 16)
zephyr_linker_section(NAME .sw_isr_table_pad GROUP TEXT_REGION NOINPUT ALIGN 4 MIN_SIZE 1024)

include(${COMMON_ZEPHYR_LINKER_DIR}/common-rom.cmake)
# kernel/init.c bounds the SMP level with __init_end, which common-rom.cmake does not place.
zephyr_linker_section_configure(SECTION init SYMBOLS __init_end)
zephyr_iterable_section(NAME sof_uuid_entry KVMA RAM_REGION GROUP RODATA_REGION)

# device_api_area is contributed by the generated device-api-sections.cmake,
# which places it in RODATA_REGION.

zephyr_linker_section(NAME .rodata GROUP RAM_REGION)
zephyr_linker_section(NAME .const GROUP RAM_REGION)
zephyr_linker_section(NAME .cinit GROUP RAM_REGION)
zephyr_linker_section(NAME .init_array GROUP RAM_REGION)
zephyr_linker_section(NAME .switch GROUP RAM_REGION)

zephyr_linker_section(NAME .data GROUP RAM_REGION NOINPUT)
zephyr_linker_section_configure(SECTION .data INPUT ".data;.data.*")

include(${COMMON_ZEPHYR_LINKER_DIR}/common-ram.cmake)

zephyr_linker_section(NAME .sdata GROUP RAM_REGION)

zephyr_linker_section(NAME .bss:c7x_diag GROUP DIAG_REGION NOINPUT)
zephyr_linker_section_configure(SECTION .bss:c7x_diag INPUT ".bss:c7x_diag")

# The C7x core stacks are used on every event and therefore must be in cached DDR or L2SRAM
# for performance. Mapping it to cached DDR region, so they don't take the limited L2SRAM.
zephyr_linker_section(NAME .c7x_core_stacks GROUP RAM_REGION NOINPUT TYPE NOLOAD ALIGN 0x10000)
zephyr_linker_section_configure(SECTION .c7x_core_stacks
                                INPUT ".c7x_ecsp_area;.c7x_tcsp_area;.c7x_isr_stack")

# type=NOLOAD is load-bearing: an explicit body otherwise defaults to
# PROGBITS and .bss gets file-backed zeros.
zephyr_linker_section(NAME .bss GROUP RAM_REGION NOINPUT TYPE NOLOAD)
zephyr_linker_section_configure(SECTION .bss INPUT ".bss;.bss.*;.sbss")

zephyr_linker_section(NAME .noinit GROUP RAM_REGION NOINPUT ALIGN 0x1000)
zephyr_linker_section_configure(SECTION .noinit INPUT ".noinit.*;.noinit")
