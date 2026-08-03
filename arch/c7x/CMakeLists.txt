# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(core)

# the unprefixed vector type names collide with Zephyr's
zephyr_compile_options(--vectypes=off)

if(NOT CONFIG_C7X_I2528_WORKAROUND)
  zephyr_compile_options(--silicon_errata_i2528=off)
endif()
