#  Copyright (c) 2026 Texas Instruments Incorporated
#  SPDX-License-Identifier: Apache-2.0

if(NOT CMAKE_DTS_PREPROCESSOR)
  find_program(CMAKE_DTS_PREPROCESSOR aarch64-zephyr-elf-gcc PATHS ${ZEPHYR_SDK_INSTALL_DIR}/gnu/aarch64-zephyr-elf/bin NO_DEFAULT_PATH)
endif()

if(NOT CMAKE_DTS_PREPROCESSOR)
  message(FATAL_ERROR "Zephyr was unable to find \`aarch64-zephyr-elf-gcc\` for DTS preprocessing")
endif()

find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}cl7x PATHS ${TOOLCHAIN_HOME}/bin NO_DEFAULT_PATH)
if(CMAKE_C_COMPILER STREQUAL CMAKE_C_COMPILER-NOTFOUND)
  message(FATAL_ERROR "Zephyr was unable to find cl7x under ${TOOLCHAIN_HOME}/bin")
endif()

# --version is NOT usable here: cl7x reports it as an invalid option, prints no
# banner and still exits 0, so the peer form would fail open.
execute_process(COMMAND ${CMAKE_C_COMPILER} --compiler_revision
  RESULT_VARIABLE cl7x_probe_result
  OUTPUT_VARIABLE CL7X_COMPILER_VERSION
  ERROR_VARIABLE  cl7x_probe_error
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT cl7x_probe_result EQUAL 0 OR CL7X_COMPILER_VERSION STREQUAL "")
  message(FATAL_ERROR
    "cl7x found at '${CMAKE_C_COMPILER}' but could not be run.\n"
    "Check file permissions and the TI license configuration.\n"
    "${cl7x_probe_error}")
endif()

if(CL7X_COMPILER_VERSION VERSION_LESS ${CL7X_MINIMUM_REQUIRED_VERSION})
  message(FATAL_ERROR
    "cl7x ${CL7X_COMPILER_VERSION} is too old; "
    "${CL7X_MINIMUM_REQUIRED_VERSION} or newer is required.")
endif()

message(STATUS "Found TI C7000 CGT ${CL7X_COMPILER_VERSION}")
