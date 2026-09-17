# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

set(CMAKE_LINKER "${TOOLCHAIN_HOME}/bin/cl7x")
set(CMAKE_C_LINK_EXECUTABLE
  "<CMAKE_C_COMPILER> --run_linker <OBJECTS> -o <TARGET> \
<CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE
  "<CMAKE_CXX_COMPILER> --run_linker <OBJECTS> -o <TARGET> \
<CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>")

# --retain lists symbols reached only by name. Entries must stay here to survive the pre-link pass.
set(CMAKE_EXE_LINKER_FLAGS_INIT
  "--ram_model --reread_libs --warn_sections \
--map_file=zephyr_final.map \
--retain=_vector_start --retain=_iheader \
--retain=arch_irq_enable --retain=arch_irq_disable \
--retain=arch_printk_char_out \
--retain=c7x_print_mbox_ready \
--retain=bg_thread_main \
--retain=z_impl_k_thread_abort \
--retain=__c7xabi_divull \
--retain=c7x_clec_irq_enable \
--retain=c7x_mmu_mair_set \
--retain=c7x_mmu_tcr_set \
--retain=c7x_mmu_tbr0_set \
--retain=c7x_mmu_tlb_inv \
--retain=c7x_mmu_enable")

include(${CMAKE_CURRENT_LIST_DIR}/rts_members.cmake)

# Empty on purpose. Its only job upstream is pulling offsets.o into the image for
# _OffsetAbsSyms, and adding a translation unit here moves the image layout.
macro(toolchain_ld_force_undefined_symbols)
endmacro()

macro(configure_linker_script linker_script_gen linker_pass_define)
  set(extra_dependencies ${ARGN})

  set(cmake_linker_script_settings
      ${PROJECT_BINARY_DIR}/include/generated/ld_script_settings_${linker_pass_define}.cmake
  )
  zephyr_linker_generate_linker_settings_file(${cmake_linker_script_settings})

  add_custom_command(
    OUTPUT ${linker_script_gen}
    DEPENDS ${cmake_linker_script_settings} ${DEVICE_API_LD_TARGET} ${extra_dependencies}
    COMMAND ${CMAKE_COMMAND}
            -C ${cmake_linker_script_settings}
            -DPASS="${linker_pass_define}"
            -DOUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/${linker_script_gen}
            -P ${ZEPHYR_BASE}/cmake/linker/cl7x/cmd_script.cmake
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/zephyr_final.map
    COMMENT "cl7x: generating linker script ${linker_script_gen}"
  )
  #  No VERBATIM: -DPASS="${linker_pass_define}" needs shell-level quoting to
  #  survive as one argument, as armlink and iar rely on.
endmacro()

function(toolchain_ld_link_elf)
  cmake_parse_arguments(
    TOOLCHAIN_LD_LINK_ELF
    ""
    "TARGET_ELF;OUTPUT_MAP;LINKER_SCRIPT"
    "LIBRARIES_PRE_SCRIPT;LIBRARIES_POST_SCRIPT;DEPENDENCIES"
    ${ARGN}
  )

  if("${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF}" MATCHES "^zephyr_pre" AND TARGET isr_tables)
    add_custom_command(
      TARGET ${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF} PRE_LINK
      COMMAND ${CL7X_TOOLCHAIN_PATH}/bin/ar7x r
              $<TARGET_FILE:isr_tables> $<TARGET_OBJECTS:isr_tables>
      COMMENT "cl7x: restoring the isr_tables placeholder for the pre-link pass"
      COMMAND_EXPAND_LISTS VERBATIM
    )
    add_custom_command(
      TARGET ${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF} POST_BUILD
      COMMAND ${CL7X_TOOLCHAIN_PATH}/bin/ar7x d $<TARGET_FILE:isr_tables> isr_tables.c.obj
      COMMENT "cl7x: dropping the isr_tables placeholder before the final link"
      VERBATIM
    )
  endif()

  list(APPEND TOOLCHAIN_LD_LINK_ELF_LIBRARIES_PRE_SCRIPT "--undef_sym=main")
  # no --whole-archive in cl7x. One reference keeps configs.c and every CONFIG_ symbol
  list(APPEND TOOLCHAIN_LD_LINK_ELF_LIBRARIES_PRE_SCRIPT "--undef_sym=CONFIG_ARCH")
  #  Stopgap for the missing --whole-archive. Nothing references thread_info.c, so name its data.
  if(CONFIG_DEBUG_THREAD_INFO)
    list(APPEND TOOLCHAIN_LD_LINK_ELF_LIBRARIES_PRE_SCRIPT "--undef_sym=_kernel_thread_info_offsets")
  endif()
  if(CONFIG_C7X_FAULT_INJECT)
    list(APPEND TOOLCHAIN_LD_LINK_ELF_LIBRARIES_PRE_SCRIPT "--undef_sym=c7x_inject_arm")
  endif()

  target_link_libraries(
    ${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF}
    ${TOOLCHAIN_LD_LINK_ELF_LIBRARIES_PRE_SCRIPT}
    ${TOOLCHAIN_LD_LINK_ELF_LINKER_SCRIPT}
    # no --whole-archive in cl7x. App objects that only fill iterable sections are never extracted
    $<TARGET_OBJECTS:app>
    ${WHOLE_ARCHIVE_LIBS}
    ${NO_WHOLE_ARCHIVE_LIBS}
    $<TARGET_OBJECTS:${OFFSETS_LIB}>
    ${TOOLCHAIN_LD_LINK_ELF_LIBRARIES_POST_SCRIPT}
    ${CL7X_RTS_OBJECTS}
    ${TOOLCHAIN_LD_LINK_ELF_DEPENDENCIES}
  )
endfunction()

include(${ZEPHYR_BASE}/cmake/linker/ld/target_relocation.cmake)
include(${ZEPHYR_BASE}/cmake/linker/ld/target_configure.cmake)
