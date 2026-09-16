# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(./common)
add_subdirectory_ifdef(CONFIG_SOC_SERIES_AM62X_C7X ./am62x/c7x)
