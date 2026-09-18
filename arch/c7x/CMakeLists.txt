# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(core)

# --vectypes=off removes the unprefixed vector type names (int4, float8); the
# double-underscore forms remain available (SPRUIG8).
zephyr_compile_options(--vectypes=off)

# on by default, off with CONFIG_C7X_I2528_WORKAROUND=n.
if(NOT CONFIG_C7X_I2528_WORKAROUND)
  zephyr_compile_options(--silicon_errata_i2528=off)
endif()
