.. _toolchain_ti_c7000_cgt:

TI C7000 Code Generation Tools (cl7x)
#####################################

#. You need to have the `TI C7000 Code Generation Tools
   <https://www.ti.com/tool/C7000-CGT>`_ installed on your host. The toolchain
   targets the C7x DSP found in AM62D, AM62A, J721E and related SoCs.

#. You need to have :ref:`Zephyr SDK <toolchain_zephyr_sdk>` installed on your host.

   .. note::
      The Zephyr SDK is used as a source of host tools such as the device tree
      compiler (DTC) and the GNU preprocessor. Even though cl7x builds the
      Zephyr image, the GNU preprocessor is still used for device tree
      preprocessing, and GNU binutils for ``readelf``/``objcopy`` steps.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``cl7x``.
   - Set :envvar:`CL7X_TOOLCHAIN_PATH` to the toolchain installation directory.

   .. tip::
      The TI MCU+ SDK exports :envvar:`CGT_TI_C7000_PATH` for the same
      installation. If :envvar:`CL7X_TOOLCHAIN_PATH` is unset, that variable is
      used instead.

#. To check that you have set these variables correctly in your current
   environment, follow this example shell session (the
   :envvar:`CL7X_TOOLCHAIN_PATH` value may be different on your system):

   .. code-block:: console

      $ echo $ZEPHYR_TOOLCHAIN_VARIANT
      cl7x
      $ echo $CL7X_TOOLCHAIN_PATH
      /home/you/ti/ti-cgt-c7000_5.0.2.LTS

   .. code-block:: batch

      # Windows
      set ZEPHYR_TOOLCHAIN_VARIANT=cl7x
      set CL7X_TOOLCHAIN_PATH=C:\ti\ti-cgt-c7000_5.0.2.LTS
      echo %CL7X_TOOLCHAIN_PATH%

Silicon version
***************

The SoC selects the C7x core, for example :kconfig:option:`CONFIG_CPU_C7504`,
and that CPU symbol sets the compiler's ``--silicon_version`` (``-mv``) option.

Limitations
***********

cl7x is not a GCC- or Clang-compatible driver. The port therefore:

* uses TI ``.cmd`` linker command files rather than GNU linker scripts,
* emits object files with a ``.obj`` extension,
* requires ``--c11`` for ``_Static_assert``,
* has no GCC extended inline assembly, so absolute symbols are encoded as
  sized ``char`` arrays by :zephyr_file:`include/zephyr/toolchain/cl7x.h`, which
  :zephyr_file:`scripts/build/gen_offset_header.py` decodes for ``EM_TI_C7X``,
* leaves ``CONFIG_GNU_C_EXTENSIONS`` and ``CONFIG_THREAD_LOCAL_STORAGE`` unavailable,
  and defaults ``CONFIG_BUILD_OUTPUT_STRIP_PATHS`` to ``n``, as cl7x has no
  ``-fmacro-prefix-map``,
* assembles ``.S`` files with the compiler driver, which does not run the C
  preprocessor over them, so ``CONFIG_`` symbols reach assembly as
  ``--asm_define`` built from :file:`autoconf.h` at configure time and are
  tested with an assembler conditional (``.if $isdefed("CONFIG_...")``) rather
  than ``#ifdef``. :zephyr_file:`cmake/compiler/cl7x/compiler_flags.cmake`
  errors out if :file:`autoconf.h` cannot be read, because a skipped pass would
  silently send every such conditional down its false arm,

cl7x usually reports an unsupported option as a warning and still exits zero
(``-Wno-vla`` is an exception and is a hard error), so
``zephyr_cc_option()`` cannot detect one by invoking the compiler. The options
it rejects are listed in ``C_EXCLUDED_OPTIONS`` and its per-language siblings in
:zephyr_file:`cmake/compiler/cl7x/compiler_flags.cmake`.

The C7000 CGT has no ``objcopy`` equivalent, so the GNU binutils from the Zephyr
SDK perform the ELF steps. TI's ``ofd7x``, ``strip7x`` and ``nm7x`` do not accept
GNU syntax and are exposed as ``CL7X_OFD7X``, ``CL7X_STRIP7X`` and ``CL7X_NM7X``
rather than being substituted for ``CMAKE_OBJDUMP``, ``CMAKE_STRIP`` and
``CMAKE_NM``.

Additional limitations, all verified against CGT 5.0.2:

* no newlib and no picolibc -- ``CONFIG_MINIMAL_LIBC`` only,
* no ``alloca()``, so logging is limited to ``CONFIG_LOG_MODE_MINIMAL``,
* no ``__builtin_ffs``; C7x supplies its own ``find_msb_set()`` and
  ``find_lsb_set()`` on the LMBD1 instruction in
  :zephyr_file:`include/zephyr/arch/c7x/ffs.h`,
* the ``cleanup`` attribute is accepted with a warning and then dropped, so the
  handler never runs,
* no ``--gc-sections`` equivalent, which is why the toolchain retains entry
  points explicitly,
* 5.0.2 is the minimum; the build refuses anything older.
