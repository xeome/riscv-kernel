#!/bin/bash
set -xue

# QEMU path
QEMU=qemu-system-riscv32

# run QEMU
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
    -kernel kernel.elf
