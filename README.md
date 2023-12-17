# riscv-kernel

This is a simple toy kernel for RISC-V.

---

## Table of Contents

- [riscv-kernel](#riscv-kernel)
  - [Table of Contents](#table-of-contents)
  - [Features](#features)
  - [Dependencies](#dependencies)
  - [Run](#run)
  - [Acknowledgements](#acknowledgements)

---

## Features

- [x] Printf
- [x] Context switching
- [x] Exception handling
- [x] Memory allocation
- [x] Page tables
- [x] Virtual memory
- [x] Syscalls  
- [x] User mode
- [x] Interactive shell
- [x] Virt-IO basic driver
- [x] Filesystem

## Dependencies

- clang
- qemu-system-riscv32
- llvm-obj-copy
- lld

For archlinux users:

```bash
sudo pacman -S clang qemu-system-riscv llvm lld
```

## Run

```bash
make && ./run.sh
```

---

## Acknowledgements

- <https://seiya.me/>
- <https://clang.llvm.org/docs/LanguageExtensions.html>
- <https://github.com/riscv-software-src/opensbi>
- <https://github.com/riscv-non-isa/riscv-sbi-doc>
