# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

set(CL7X_RTS_LIB "${CL7X_TOOLCHAIN_PATH}/lib/rts${CL7X_SILICON_VERSION}_le.lib")

set(CL7X_RTS_MEMBERS
  imath64.c.obj
  e_log2.c.obj
  divd.c.obj
  frcdivd.c.obj
  frcmpyd.c.obj
  frcmpyd_div.c.obj
  fixdli.c.obj
  fixdlli.c.obj
  fixfli.c.obj
  fixflli.c.obj
  e_fmod.c.obj
  s_floor.c.obj
  s_ceilf.c.obj   # AUDIOLIB asrc calls ceilf()
  s_ceil.c.obj    # and AUDIOLIB ssrc calls ceil()
  fixfu.c.obj     # ssrc converts float to unsigned
  e_exp.c.obj     # AUDIOLIB dB20/undB20 call exp()
  e_log10.c.obj   # and log10()
  e_log10f.c.obj  # and log10f()
  typeinfo.c.obj  # defines the __cxxabiv1 type-info vtables the TISP C++ TUs need
  rtti.c.obj
  stdlib_typeinfo.cpp.obj
  vla_alloc.c.obj
  error.c.obj
  divf.c.obj
  s_sinf.c.obj
  s_cosf.c.obj
  k_sinf.c.obj
  # double sin/cos. The rfft node builds its twiddles in double
  s_sin.c.obj
  s_cos.c.obj
  k_sin.c.obj
  k_cos.c.obj
  e_rem_pio2.c.obj
  k_cosf.c.obj
  e_rem_pio2f.c.obj
  e_expf.c.obj
  e_powf.c.obj    # AUDIOLIB muteNCh calls powf() for its exponential fade
  s_scalbnf.c.obj # and powf() calls scalbnf(), which calls copysignf()
  s_copysignf.c.obj
  s_scalbn.c.obj
  s_copysign.c.obj
  k_rem_pio2.c.obj
  errno.c.obj
  fixfull.c.obj   # TISP MultiplexerSmooth converts float to unsigned long long
  fltullf.c.obj   # and back
  fixful.c.obj
  fltulf.c.obj
)


set(CL7X_RTS_DIR "${CMAKE_BINARY_DIR}/cl7x_rts")

# Re-extract when the library or the member list changes. Keying only on "the
# file is already there" serves members from the previous toolchain after an
# upgrade in the same build directory, and nothing says so.
file(MD5 "${CL7X_RTS_LIB}" _cl7x_rts_md5)
string(REPLACE ";" " " _cl7x_rts_list "${CL7X_RTS_MEMBERS}")
set(_cl7x_rts_stamp_want "${_cl7x_rts_md5} ${_cl7x_rts_list}")
set(_cl7x_rts_stamp "${CL7X_RTS_DIR}/.rts-stamp")
set(_cl7x_rts_stamp_have "")
if(EXISTS "${_cl7x_rts_stamp}")
  file(READ "${_cl7x_rts_stamp}" _cl7x_rts_stamp_have)
endif()
if(NOT _cl7x_rts_stamp_have STREQUAL _cl7x_rts_stamp_want)
  file(REMOVE_RECURSE "${CL7X_RTS_DIR}")
  message(STATUS "cl7x: RTS library or member list changed, re-extracting")
endif()
file(MAKE_DIRECTORY "${CL7X_RTS_DIR}")

set(CL7X_RTS_OBJECTS "")
foreach(_member ${CL7X_RTS_MEMBERS})
  set(_out "${CL7X_RTS_DIR}/${_member}")
  if(NOT EXISTS "${_out}")
    execute_process(
      COMMAND "${CL7X_TOOLCHAIN_PATH}/bin/ar7x" x "${CL7X_RTS_LIB}" "${_member}"
      WORKING_DIRECTORY "${CL7X_RTS_DIR}"
      RESULT_VARIABLE _rc
      OUTPUT_QUIET ERROR_VARIABLE _err)
    if(NOT _rc EQUAL 0 OR NOT EXISTS "${_out}")
      message(FATAL_ERROR
        "cl7x: failed to extract ${_member} from ${CL7X_RTS_LIB}\n${_err}")
    endif()
  endif()
  list(APPEND CL7X_RTS_OBJECTS "${_out}")
endforeach()

file(WRITE "${_cl7x_rts_stamp}" "${_cl7x_rts_stamp_want}")
list(LENGTH CL7X_RTS_OBJECTS _cl7x_rts_n)
message(STATUS "cl7x: ${_cl7x_rts_n} RTS members extracted from ${CL7X_RTS_LIB}")
