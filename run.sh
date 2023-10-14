#!/bin/bash
set -xue

# QEMU path
QEMU=qemu-system-riscv32

# run QEMU
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
    -drive id=drive0,file=disk.tar,format=raw \
    -device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0 \
    -kernel kernel.elf
