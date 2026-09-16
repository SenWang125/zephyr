#  Copyright (c) 2026 Texas Instruments Incorporated
#  SPDX-License-Identifier: Apache-2.0

# CGT has no objcopy and its tools reject GNU syntax
foreach(_triple aarch64-zephyr-elf riscv64-zephyr-elf)
  set(CL7X_GNU_HOME ${ZEPHYR_SDK_INSTALL_DIR}/gnu/${_triple}/bin)
  set(CL7X_GNU_PREFIX ${_triple}-)
  if(EXISTS ${CL7X_GNU_HOME}/${CL7X_GNU_PREFIX}readelf)
    break()
  endif()
endforeach()
find_program(CMAKE_OBJCOPY ${CL7X_GNU_PREFIX}objcopy PATHS ${CL7X_GNU_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_OBJDUMP ${CL7X_GNU_PREFIX}objdump PATHS ${CL7X_GNU_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_READELF ${CL7X_GNU_PREFIX}readelf PATHS ${CL7X_GNU_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_NM      ${CL7X_GNU_PREFIX}nm      PATHS ${CL7X_GNU_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_STRIP   ${CL7X_GNU_PREFIX}strip   PATHS ${CL7X_GNU_HOME} NO_DEFAULT_PATH)
include(${ZEPHYR_BASE}/cmake/bintools/gnu/target_bintools.cmake)

# TI's native tools, for callers that want TI-format output.
foreach(_tool ofd7x strip7x nm7x)
  if(EXISTS "${TOOLCHAIN_HOME}/bin/${_tool}")
    string(TOUPPER ${_tool} _tool_var)
    set(CL7X_${_tool_var} "${TOOLCHAIN_HOME}/bin/${_tool}" CACHE FILEPATH
        "TI C7000 CGT ${_tool}")
  endif()
endforeach()

set(CMAKE_AR "${TOOLCHAIN_HOME}/bin/ar7x")
set(CMAKE_RANLIB true)
set(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> r <TARGET> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> r <TARGET> <OBJECTS>")
set(CMAKE_C_ARCHIVE_APPEND   "<CMAKE_AR> r <TARGET> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> r <TARGET> <OBJECTS>")
set(CMAKE_C_ARCHIVE_FINISH   true)
set(CMAKE_CXX_ARCHIVE_FINISH true)
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
  CMAKE_AR CMAKE_RANLIB
  CMAKE_C_ARCHIVE_CREATE CMAKE_C_ARCHIVE_APPEND CMAKE_C_ARCHIVE_FINISH
  CMAKE_CXX_ARCHIVE_CREATE CMAKE_CXX_ARCHIVE_APPEND CMAKE_CXX_ARCHIVE_FINISH)
