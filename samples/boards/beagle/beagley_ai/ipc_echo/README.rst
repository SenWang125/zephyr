.. zephyr:code-sample:: beagley-ai-ipc-echo
   :name: BeagleY-AI C7x IPC echo
   :relevant-api: ipm_interface mbox_interface

   Echo rpmsg messages back to Linux from the C7x DSP.

Overview
********

Runs on the first C7x DSP of the TI AM67A (J722S) on a BeagleY-AI, and echoes
every message Linux sends to the ``beagley-ipc-echo`` service back to its
sender.

Linux loads the firmware through remoteproc and owns the vrings; the DSP takes
their addresses from the resource table the kernel updates in place. The
mailbox doorbell is cluster 2, reached through the generic
:dtcompatible:`zephyr,mbox-ipm` adaptor over the OMAP mailbox driver.

The DSP has no UART of its own on this board, because Linux owns UART0. The
console is the remoteproc trace buffer instead, readable from Linux at
``/sys/kernel/debug/remoteproc/remoteprocN/trace0``.

Requirements
************

A BeagleY-AI running a Linux kernel whose device tree enables the C7x_0
remoteproc node. ``k3-am67a-beagley-ai.dts`` includes
``k3-j722s-ti-ipc-firmware.dtsi``, which does this and reserves the DDR the
firmware links into.

Building
********

.. zephyr-app-commands::
   :zephyr-app: samples/boards/beagle/beagley_ai/ipc_echo
   :board: beagley_ai/j722s/c71_0
   :goals: build
   :compact:

The C7x is built with the TI C7000 Code Generation Tools, see
:ref:`toolchain_ti_c7000_cgt`.

Running
*******

Copy the ELF to the firmware name the Linux device tree gives the C7x
remoteproc node and restart the core:

.. code-block:: console

   cp build/zephyr/zephyr.elf /lib/firmware/j722s-c71_0-fw
   echo stop  > /sys/class/remoteproc/remoteprocN/state
   echo start > /sys/class/remoteproc/remoteprocN/state

Pick ``remoteprocN`` by name rather than by index, because the numbering
depends on probe order:

.. code-block:: console

   grep -l 7e000000.dsp /sys/class/remoteproc/remoteproc*/name

On ``stop`` the kernel sends a shutdown request through the mailbox. The
sample acknowledges it from the mailbox interrupt and idles the core, and the
kernel then resets it. After a fatal error the core acknowledges the request
from its halt loop instead
(:kconfig:option:`CONFIG_SOC_FAMILY_TI_K3_C7X_FATAL_SHUTDOWN_ACK`), so ``stop``
also succeeds on a core that has crashed.

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build ... ***
   I: BeagleY-AI C7x IPC echo starting
   I: announced "beagley-ipc-echo", waiting for messages
   I: echoing 16 bytes
   I: shutdown requested

Linux sees the service appear on the rpmsg bus:

.. code-block:: console

   # ls /sys/bus/rpmsg/devices/
   virtio0.beagley-ipc-echo.-1.1024
