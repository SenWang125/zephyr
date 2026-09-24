# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

set_property(TARGET linker PROPERTY undefined "--undef_sym=")

set_property(TARGET linker PROPERTY no_warn_rwx_segments)

# INHERITED from the compiler target, so --opt_level lands after --run_linker
# unless zeroed here. iar and armlink do the same.
foreach(_opt no_optimization optimization_debug optimization_speed
             optimization_size optimization_size_aggressive)
  set_property(TARGET linker PROPERTY ${_opt} "")
endforeach()

set_property(TARGET linker PROPERTY devices_start_symbol "_device_list_start")
