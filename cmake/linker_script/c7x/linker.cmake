# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0
#
#  Rendered into a TI .cmd by cmake/linker/cl7x/cmd_script.cmake.
#
#  The firmware owns 0x99900000..0x9A800000 and nothing else. Every region is
#  inside that window and STACKS ends exactly on 0x9A800000. The Linux-SDK
#  reference (ipc_rpmsg_echo_linux/am62dx-evm/c75ss0-0_freertos/ti-c7000/
#  linker.cmd) partitions the same window with the same anchors.

zephyr_linker(ENTRY ${CONFIG_KERNEL_ENTRY})

set(COMMON_ZEPHYR_LINKER_DIR ${ZEPHYR_BASE}/cmake/linker_script/common)

zephyr_linker_include_var(VAR C7X_CMD_HEAP  VALUE 0x10000)
zephyr_linker_include_var(VAR C7X_CMD_STACK VALUE 0x4000)
zephyr_linker_include_var(VAR C7X_CMD_DIAG_SUPPRESS VALUE "10068 10063")

#  cl7x drops these without a reference; KEEP inputs are retained by the generator.
zephyr_linker_include_var(VAR C7X_CMD_RETAIN VALUE
  "*(.__static_thread_data.static*) \
*(.data:c7x_mmu_tables) \
*(.bss:c7x_ecsp_area) \
*(.bss:c7x_tcsp_area)")

zephyr_linker_include_var(VAR C7X_CMD_GROUPS VALUE "MMU_TABLES")

#  TI attributes the generic model has no equivalent for, keyed by section
#  name with non-alphanumerics folded to '_'.
zephyr_linker_include_var(VAR C7X_TI_ATTR_l2sramData VALUE "RUN_START(__bss_c7x_start)")
zephyr_linker_include_var(VAR C7X_TI_ATTR_bss_c7x_ecsp_area VALUE "RUN_START(__c7x_ecsp_base)")
zephyr_linker_include_var(VAR C7X_TI_ATTR_bss_c7x_tcsp_area VALUE "RUN_START(__c7x_tcsp_base)")
zephyr_linker_include_var(VAR C7X_TI_ATTR_bss_c7x_diag VALUE "RUN_END(__bss_c7x_end)")
zephyr_linker_include_var(VAR C7X_TI_ATTR_bss
  VALUE "RUN_START(__bss_start) RUN_END(__bss_end) RUN_START(__bss_tcsp_boot_start)")
zephyr_linker_include_var(VAR C7X_TI_ATTR_bss_c7x_tcsp_boot VALUE "RUN_END(__bss_tcsp_boot_end)")

dt_comp_path(c7x_memory_regions COMPATIBLE "zephyr,memory-region")
foreach(path IN LISTS c7x_memory_regions)
  zephyr_linker_dts_memory(PATH ${path})
endforeach()

if(CONFIG_MMU)
  #  kernel/mmu.c pins the page frames from z_mapped_start to z_mapped_end: the whole zephyr,sram.
  dt_chosen(c7x_sram_path PROPERTY "zephyr,sram")
  dt_reg_addr(c7x_sram_base PATH ${c7x_sram_path})
  dt_reg_size(c7x_sram_size PATH ${c7x_sram_path})
  math(EXPR c7x_sram_end "${c7x_sram_base} + ${c7x_sram_size}" OUTPUT_FORMAT HEXADECIMAL)
  zephyr_linker_symbol(SYMBOL z_mapped_start EXPR "${c7x_sram_base}")
  zephyr_linker_symbol(SYMBOL z_mapped_end EXPR "${c7x_sram_end}")
endif()

#  Scratch for the pre-pass .intList declared by arch/common; discarded before
#  the final link.
zephyr_linker_memory(NAME IDT_LIST  FLAGS wx      START 0xFFFF8000 SIZE 2K)

zephyr_linker_group(NAME IPC_RSC_REGION LMA IPC_RSC VMA IPC_RSC)
zephyr_linker_group(NAME TRACE_REGION   LMA TRACE   VMA TRACE)
zephyr_linker_group(NAME DIAG_REGION    LMA DIAG    VMA DIAG)
zephyr_linker_group(NAME VECTORS_REGION LMA VECTORS VMA VECTORS)
zephyr_linker_group(NAME L2SRAM_REGION  LMA L2SRAM  VMA L2SRAM)
zephyr_linker_group(NAME L2AUX_REGION   LMA L2AUX   VMA L2AUX)
zephyr_linker_group(NAME STACKS_REGION  LMA STACKS  VMA STACKS)

#  Group order is emission order, and TI allocates a region in listed order.
zephyr_linker_group(NAME ROM_REGION    LMA SRAM VMA SRAM)
zephyr_linker_group(NAME TEXT_REGION   GROUP ROM_REGION)
zephyr_linker_group(NAME RODATA_REGION GROUP ROM_REGION)
zephyr_linker_group(NAME RAM_REGION    LMA SRAM VMA SRAM)
zephyr_linker_group(NAME DATA_REGION   GROUP RAM_REGION)
zephyr_linker_group(NAME NOINIT_REGION GROUP RAM_REGION)
zephyr_linker_group(NAME MMU_TABLES    LMA SRAM VMA SRAM)

zephyr_linker_section(NAME .resource_table GROUP IPC_RSC_REGION NOINPUT)
zephyr_linker_section_configure(SECTION .resource_table INPUT ".resource_table"
                                SYMBOLS __RESOURCE_TABLE)

zephyr_linker_section(NAME .bss.debug_mem_trace_buf GROUP TRACE_REGION NOINPUT
                      MIN_SIZE 0xF400)

zephyr_linker_section(NAME .vecs GROUP VECTORS_REGION ALIGN 0x400000)

zephyr_linker_section(NAME .text:_c_int00_secure GROUP TEXT_REGION NOINPUT ALIGN 0x200000)
zephyr_linker_section_configure(SECTION .text:_c_int00_secure INPUT ".text:_c_int00_secure")

zephyr_linker_section(NAME .text:c7x_startup GROUP TEXT_REGION NOINPUT)
zephyr_linker_section_configure(SECTION .text:c7x_startup INPUT ".text:c7x_startup")

#  ALIGN(0x200000) on the entry and .text keeps the layout insensitive to size.
zephyr_linker_section(NAME .text GROUP TEXT_REGION NOINPUT ALIGN 0x200000)
zephyr_linker_section_configure(SECTION .text
                                INPUT ".text;.text.*")

#  arch/common owns .intList for the earlier passes, where it goes to IDT_LIST.
zephyr_linker_section(NAME .intList GROUP TEXT_REGION NOINPUT PASS LINKER_ZEPHYR_FINAL)
zephyr_linker_section_configure(SECTION .intList INPUT ".irq_info*;.intList*"
                                PASS LINKER_ZEPHYR_FINAL)

zephyr_linker_section(NAME .gnu.linkonce.irq_vector_table GROUP TEXT_REGION NOINPUT ALIGN 16)
zephyr_linker_section(NAME .gnu.linkonce.sw_isr_table GROUP TEXT_REGION NOINPUT ALIGN 16)
zephyr_linker_section(NAME .sw_isr_table_pad GROUP TEXT_REGION NOINPUT ALIGN 4 MIN_SIZE 1024)

include(${COMMON_ZEPHYR_LINKER_DIR}/common-rom.cmake)
#  kernel/init.c bounds the SMP level with __init_end, which common-rom.cmake does not place.
zephyr_linker_section_configure(SECTION init SYMBOLS __init_end)
zephyr_iterable_section(NAME sof_uuid_entry KVMA RAM_REGION GROUP RODATA_REGION)

zephyr_linker_section(NAME .z_deferred_init GROUP TEXT_REGION NOINPUT)
zephyr_linker_section_configure(SECTION .z_deferred_init INPUT ".z_deferred_init*"
                                SYMBOLS __deferred_init_list_start __deferred_init_list_end)

#  device_api_area is contributed by the generated device-api-sections.cmake,
#  which places it in RODATA_REGION.

if(CONFIG_ZTEST)
  foreach(name ztest_expected_result_entry ztest_suite_node ztest_unit_test
               ztest_param_inst ztest_test_rule)
    zephyr_linker_section(NAME .z_${name} GROUP TEXT_REGION NOINPUT)
    zephyr_linker_section_configure(SECTION .z_${name} INPUT "._${name}.static*"
                                    SYMBOLS _${name}_list_start _${name}_list_end)
  endforeach()
endif()

zephyr_linker_section(NAME .rodata GROUP RAM_REGION)
zephyr_linker_section(NAME .const GROUP RAM_REGION)
zephyr_linker_section(NAME .cinit GROUP RAM_REGION)
zephyr_linker_section(NAME .init_array GROUP RAM_REGION)
zephyr_linker_section(NAME .switch GROUP RAM_REGION)

zephyr_linker_section(NAME .z_interrupt_stacks GROUP RAM_REGION NOINPUT)
zephyr_linker_section_configure(SECTION .z_interrupt_stacks SYMBOLS __bss_pre_data_start)
zephyr_linker_section_configure(SECTION .z_interrupt_stacks INPUT ".z_interrupt_stacks*"
                                SYMBOLS _interrupt_stack_start _interrupt_stack_end)
zephyr_linker_section_configure(SECTION .z_interrupt_stacks SYMBOLS __bss_pre_data_end)

zephyr_linker_section(NAME .data GROUP RAM_REGION NOINPUT)
zephyr_linker_section_configure(SECTION .data INPUT ".data;.data.*")

include(${COMMON_ZEPHYR_LINKER_DIR}/common-ram.cmake)

zephyr_linker_section(NAME .data:c7x_mmu_tables GROUP RAM_REGION ALIGN 0x1000)
zephyr_linker_section(NAME .sdata GROUP RAM_REGION)

#  The ISR stack lives in L2SRAM, aligned to its 4 KiB page.
zephyr_linker_section(NAME .c7x_isr_stack_l2 GROUP L2SRAM_REGION NOINPUT
                      TYPE NOLOAD ALIGN 0x1000)
zephyr_linker_section_configure(SECTION .c7x_isr_stack_l2 INPUT ".c7x_isr_stack_l2")
zephyr_linker_section(NAME .l2sramData GROUP L2SRAM_REGION TYPE NOLOAD)

zephyr_linker_section(NAME .bss:c7x_ecsp_area GROUP STACKS_REGION NOINPUT ALIGN 0x10000)
zephyr_linker_section_configure(SECTION .bss:c7x_ecsp_area INPUT ".bss:c7x_ecsp_area")

#  __TCSP starts immediately above the ECSP area.
zephyr_linker_section(NAME .bss:c7x_tcsp_area GROUP STACKS_REGION NOINPUT ALIGN 0x2000)
zephyr_linker_section_configure(SECTION .bss:c7x_tcsp_area INPUT ".bss:c7x_tcsp_area")

# NOLOAD like every L2 section: an L2 PT_LOAD crashes remoteproc (invariant 10).
zephyr_linker_section(NAME .l2auxData GROUP L2AUX_REGION NOINPUT TYPE NOLOAD ALIGN 64)
zephyr_linker_section_configure(SECTION .l2auxData INPUT ".l2auxData")

zephyr_linker_section(NAME .bss:c7x_diag GROUP DIAG_REGION NOINPUT)
zephyr_linker_section_configure(SECTION .bss:c7x_diag INPUT ".bss:c7x_diag")

zephyr_linker_section(NAME .thames_pt_pool GROUP STACKS_REGION ALIGN 0x1000)

#  type=NOLOAD is load-bearing: an explicit body otherwise defaults to
#  PROGBITS and .bss gets file-backed zeros.
zephyr_linker_section(NAME .bss GROUP RAM_REGION NOINPUT TYPE NOLOAD)
zephyr_linker_section_configure(SECTION .bss INPUT ".bss;.bss.*;.bss:*;.sbss")

zephyr_linker_section(NAME .bss:c7x_tcsp_boot GROUP RAM_REGION NOINPUT ALIGN 0x2000)
zephyr_linker_section_configure(SECTION .bss:c7x_tcsp_boot INPUT ".bss:c7x_tcsp_boot")

zephyr_linker_section(NAME .noinit GROUP RAM_REGION NOINPUT ALIGN 0x1000)
zephyr_linker_section_configure(SECTION .noinit INPUT ".noinit.*;.noinit")

zephyr_linker_section(NAME .kstackmem GROUP RAM_REGION NOINPUT ALIGN 64)
zephyr_linker_section_configure(SECTION .kstackmem INPUT ".kstackmem;.kstackmem.*")

zephyr_linker_section(NAME .z_kernel_ram GROUP RAM_REGION)
zephyr_linker_section(NAME .z_thread_stack GROUP RAM_REGION)
zephyr_linker_section(NAME .sysmem GROUP RAM_REGION)
zephyr_linker_section(NAME .stack GROUP RAM_REGION ALIGN 0x2000)
zephyr_linker_section(NAME .cio GROUP RAM_REGION)

foreach(table Mmu_tableArray Mmu_tableArraySlot Mmu_level1Table
              gMmu_tableArray_NS Mmu_tableArraySlot_NS Mmu_level1Table_NS)
  zephyr_linker_section(NAME .data.${table} GROUP MMU_TABLES NOINPUT NOINIT)
endforeach()
