.. zephyr:board:: am62d_evm

Overview
********

The AM62D2 EVM is a Texas Instruments evaluation module for the AM62D2 audio
processor. Zephyr runs on the C7x DSP core of the SoC as a remote processor:
Linux on the Cortex-A53 cores loads the Zephyr image with remoteproc and
talks to it over RPMsg.

This board configuration supports the C7x core with:

- C7x Compute Cluster Level-2 Event Controller (CLEC)
- DMTimer system clock
- On-chip L2 SRAM
- OMAP mailbox and OpenAMP static vrings for IPC with Linux
- TI secure proxy mailbox for TISCI
- McASP audio interface

Hardware
********

The AM62D2 SoC has a quad Cortex-A53 cluster, Cortex-R5F cores and a C7x DSP
core. The C7x has on-chip L2 SRAM and runs from a DDR region that Linux
reserves for it in its device tree.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The C7x core is built with the TI C7000 Code Generation Tools, see
:ref:`toolchain_ti_c7000_cgt`.

Flashing
========

There is no flash step. Build an application for the C7x core:

.. zephyr-app-commands::
   :app: sof/app/am62d_evm_c7x_hello
   :board: am62d_evm/am62d2/c71_0
   :goals: build
   :compact:

Copy :file:`build/zephyr/zephyr.elf` to the firmware name the Linux device
tree gives the C7x remoteproc node, then restart the core:

.. code-block:: console

   cp zephyr.elf /lib/firmware/am62d-c71_0-fw
   echo stop > /sys/class/remoteproc/remoteproc0/state
   echo start > /sys/class/remoteproc/remoteproc0/state

Console output goes to the remoteproc trace buffer, not a UART:

.. code-block:: console

   cat /sys/kernel/debug/remoteproc/remoteproc0/trace0

Debugging
=========

No west runner is integrated for the C7x core.
