# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(core)

# the ISR stack is not 64-byte aligned
zephyr_compile_options(--vectypes=off)
