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
SRC_FILES=$(shell find $(KERNEL_SRC) $(USER_SRC) -name '*.c')
SHELL_SOURCES= $(USER_SRC)/shell.c $(USER_SRC)/user.c $(KERNEL_SRC)/common.c
KERNEL_SOURCES= $(wildcard $(KERNEL_SRC)/*.c)


# Build targets
all: $(BUILD_DIR)/shell.bin.o kernel.elf

$(BUILD_DIR)/shell.bin.o: $(SHELL_SOURCES)
	$(CC) $(CFLAGS) -Wl,-Tuser.ld -o $(BUILD_DIR)/shell.elf $(SHELL_SOURCES)
	$(OBJCOPY) --set-section-flags .bss=alloc,contents -O binary $(BUILD_DIR)/shell.elf $(BUILD_DIR)/shell.bin
	$(OBJCOPY) -Ibinary -Oelf32-littleriscv $(BUILD_DIR)/shell.bin $(BUILD_DIR)/shell.bin.o

kernel.elf: $(BUILD_DIR)/shell.bin.o $(SRC_FILES)
	$(CC) $(CFLAGS) -Wl,-Tkernel.ld  -o kernel.elf \
    	$(KERNEL_SOURCES) $(BUILD_DIR)/shell.bin.o

clean:
	rm -f $(BUILD_DIR)/*.bin $(BUILD_DIR)/*.elf $(BUILD_DIR)/*.o $(BUILD_DIR)/*.map

.PHONY: all run clean
