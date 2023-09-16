# Makefile for building and running the code

# QEMU path
QEMU=qemu-system-riscv32

# Compiler and flags
CC=clang
OBJCOPY=llvm-objcopy
CFLAGS=-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32 -ffreestanding -nostdlib -I include

# Source folders
KERNEL_SRC=kernel
USER_SRC=user
BUILD_DIR=build

# Source files
#SRC_FILES=shell.c user.c common.c kernel.c
SRC_FILES=$(shell find $(KERNEL_SRC) $(USER_SRC) -name '*.c')
SHELL_SRC= $(USER_SRC)/shell.c $(USER_SRC)/user.c $(KERNEL_SRC)/common.c


# Build targets
all: shell.bin kernel.elf

shell.bin: $(SRC_FILES)
	$(CC) $(CFLAGS) -Wl,-Tuser.ld -Wl,-Map=$(BUILD_DIR)/shell.map -o $(BUILD_DIR)/shell.elf $(SHELL_SRC)
	$(OBJCOPY) --set-section-flags .bss=alloc,contents -O binary $(BUILD_DIR)/shell.elf $(BUILD_DIR)/shell.bin
	$(OBJCOPY) -Ibinary -Oelf32-littleriscv $(BUILD_DIR)/shell.bin $(BUILD_DIR)/shell.bin.o

kernel.elf: $(SRC_FILES) $(BUILD_DIR)/shell.bin.o
	$(CC) $(CFLAGS) -Wl,-Tkernel.ld -Wl,-Map=$(BUILD_DIR)/kernel.map -o kernel.elf \
    	$(KERNEL_SRC)/kernel.c $(KERNEL_SRC)/common.c $(BUILD_DIR)/shell.bin.o

clean:
	rm -f $(BUILD_DIR)/*.bin $(BUILD_DIR)/*.elf $(BUILD_DIR)/*.o $(BUILD_DIR)/*.map

.PHONY: all run clean
