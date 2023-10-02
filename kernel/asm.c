#include "kernel.h"

extern char __bss[], __bss_end[], __stack_top[];

/**
 * @brief Switches the context from the previous stack pointer to the next stack pointer.
 *
 * This function saves the current register values onto the previous stack pointer and loads the register values from
 * the next stack pointer.
 *
 * @param prev_sp Pointer to the previous stack pointer.
 * @param next_sp Pointer to the next stack pointer.
 */
__attribute__((naked)) void switch_context(uint32_t* prev_sp, uint32_t* next_sp) {
    __asm__ __volatile__(
        "addi sp, sp, -13 * 4\n"
        "sw ra,  0  * 4(sp)\n"
        "sw s0,  1  * 4(sp)\n"
        "sw s1,  2  * 4(sp)\n"
        "sw s2,  3  * 4(sp)\n"
        "sw s3,  4  * 4(sp)\n"
        "sw s4,  5  * 4(sp)\n"
        "sw s5,  6  * 4(sp)\n"
        "sw s6,  7  * 4(sp)\n"
        "sw s7,  8  * 4(sp)\n"
        "sw s8,  9  * 4(sp)\n"
        "sw s9,  10 * 4(sp)\n"
        "sw s10, 11 * 4(sp)\n"
        "sw s11, 12 * 4(sp)\n"
        "sw sp, (a0)\n"
        "lw sp, (a1)\n"
        "lw ra,  0  * 4(sp)\n"
        "lw s0,  1  * 4(sp)\n"
        "lw s1,  2  * 4(sp)\n"
        "lw s2,  3  * 4(sp)\n"
        "lw s3,  4  * 4(sp)\n"
        "lw s4,  5  * 4(sp)\n"
        "lw s5,  6  * 4(sp)\n"
        "lw s6,  7  * 4(sp)\n"
        "lw s7,  8  * 4(sp)\n"
        "lw s8,  9  * 4(sp)\n"
        "lw s9,  10 * 4(sp)\n"
        "lw s10, 11 * 4(sp)\n"
        "lw s11, 12 * 4(sp)\n"
        "addi sp, sp, 13 * 4\n"
        "ret\n");
}

/**
 * @brief This function is the entry point for user mode. It sets the value of the Supervisor Exception Program Counter (sepc) and
 * the Supervisor Status Register (sstatus) before performing a Supervisor-level return (sret) instruction to enter user mode.
 *
 * @details This function is marked with the naked attribute, which means that the function prologue and epilogue are not
 * generated by the compiler. Instead, the function must manually save and restore the registers that it uses. The function uses
 * inline assembly to set the values of the sepc and sstatus registers before performing the sret instruction.
 *
 * @note This function assumes that the USER_BASE and SSTATUS_SPIE constants are defined elsewhere in the code.
 */
__attribute__((naked)) void user_entry(void) {
    __asm__ __volatile__(
        "csrw sepc, %[sepc]\n"
        "csrw sstatus, %[sstatus]\n"
        "sret\n"
        :
        : [sepc] "r"(USER_BASE), [sstatus] "r"(SSTATUS_SPIE));
}

/**
 * @brief This function is the entry point of the kernel. It saves the current state of the registers and calls the
 * handle_trap function to handle any traps that occur. After handling the trap, it restores the saved state of the
 * registers and returns to the interrupted code.
 */
__attribute__((naked)) __attribute__((aligned(4))) void kernel_entry(void) {
    __asm__ __volatile__(
        // Exchange the kernel stack with the sscratch register.
        // tmp = sp; sp = sscratch; sscratch = tmp;
        "csrrw sp, sscratch, sp\n"

        // Allocate space on the kernel stack for the saved registers
        "addi sp, sp, -4 * 31\n"

        "sw ra,  4 * 0(sp)\n"
        "sw gp,  4 * 1(sp)\n"
        "sw tp,  4 * 2(sp)\n"
        "sw t0,  4 * 3(sp)\n"
        "sw t1,  4 * 4(sp)\n"
        "sw t2,  4 * 5(sp)\n"
        "sw t3,  4 * 6(sp)\n"
        "sw t4,  4 * 7(sp)\n"
        "sw t5,  4 * 8(sp)\n"
        "sw t6,  4 * 9(sp)\n"
        "sw a0,  4 * 10(sp)\n"
        "sw a1,  4 * 11(sp)\n"
        "sw a2,  4 * 12(sp)\n"
        "sw a3,  4 * 13(sp)\n"
        "sw a4,  4 * 14(sp)\n"
        "sw a5,  4 * 15(sp)\n"
        "sw a6,  4 * 16(sp)\n"
        "sw a7,  4 * 17(sp)\n"
        "sw s0,  4 * 18(sp)\n"
        "sw s1,  4 * 19(sp)\n"
        "sw s2,  4 * 20(sp)\n"
        "sw s3,  4 * 21(sp)\n"
        "sw s4,  4 * 22(sp)\n"
        "sw s5,  4 * 23(sp)\n"
        "sw s6,  4 * 24(sp)\n"
        "sw s7,  4 * 25(sp)\n"
        "sw s8,  4 * 26(sp)\n"
        "sw s9,  4 * 27(sp)\n"
        "sw s10, 4 * 28(sp)\n"
        "sw s11, 4 * 29(sp)\n"

        // Retrieve and save the sp when exception occurrs
        "csrr a0, sscratch\n"
        "sw a0,  4 * 30(sp)\n"

        // Reset the kernel stack
        "addi a0, sp, 4 * 31\n"
        "csrw sscratch, a0\n"

        // Call handle_trap
        "mv a0, sp\n"
        "call handle_trap\n"

        "lw ra,  4 * 0(sp)\n"
        "lw gp,  4 * 1(sp)\n"
        "lw tp,  4 * 2(sp)\n"
        "lw t0,  4 * 3(sp)\n"
        "lw t1,  4 * 4(sp)\n"
        "lw t2,  4 * 5(sp)\n"
        "lw t3,  4 * 6(sp)\n"
        "lw t4,  4 * 7(sp)\n"
        "lw t5,  4 * 8(sp)\n"
        "lw t6,  4 * 9(sp)\n"
        "lw a0,  4 * 10(sp)\n"
        "lw a1,  4 * 11(sp)\n"
        "lw a2,  4 * 12(sp)\n"
        "lw a3,  4 * 13(sp)\n"
        "lw a4,  4 * 14(sp)\n"
        "lw a5,  4 * 15(sp)\n"
        "lw a6,  4 * 16(sp)\n"
        "lw a7,  4 * 17(sp)\n"
        "lw s0,  4 * 18(sp)\n"
        "lw s1,  4 * 19(sp)\n"
        "lw s2,  4 * 20(sp)\n"
        "lw s3,  4 * 21(sp)\n"
        "lw s4,  4 * 22(sp)\n"
        "lw s5,  4 * 23(sp)\n"
        "lw s6,  4 * 24(sp)\n"
        "lw s7,  4 * 25(sp)\n"
        "lw s8,  4 * 26(sp)\n"
        "lw s9,  4 * 27(sp)\n"
        "lw s10, 4 * 28(sp)\n"
        "lw s11, 4 * 29(sp)\n"
        "lw sp,  4 * 30(sp)\n"
        "sret\n");
}

/**
 * @brief This function is the entry point of the kernel. It sets the stack pointer to the top of the stack and jumps to
 * the kernel_main function.
 *
 */
__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
    __asm__ __volatile__(
        // Set the stack pointer to the top of the stack
        "mv sp, %[stack_top]\n"
        // Jump to the kernel_main function
        "j kernel_main\n"
        :
        : [stack_top] "r"(__stack_top));  // Input constraint specifying %[stack_top] corresponds to __stack_top
}

/**
 * Executes an ecall instruction with the given arguments using inline assembler.
 * Registers a0-a7 are used to pass arguments to the ecall instruction.
 * The ecall instruction may modify any register or memory location in the system.
 *
 * @param fid The function ID to be passed as the sixth argument to the ecall instruction.
 * @param eid The extension ID to be passed as the seventh argument to the ecall instruction.
 *
 * @return A struct sbiret containing the error code and the value returned by the ecall instruction.
 */
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid) {
    // Registers a0-a7 are used to pass arguments to the ecall instruction
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;  // Function ID
    register long a7 __asm__("a7") = eid;  // Extension ID

    // Use inline assembler to execute the "ecall" instruction
    __asm__ __volatile__("ecall"
                         : "=r"(a0),
                           "=r"(a1)  // Outputs: a0 will hold the error code, a1 will hold the value
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                         : "memory");  // Clobbers: the ecall instruction may modify any register or memory location in the system
    return (struct sbiret){.error = a0, .value = a1};
}