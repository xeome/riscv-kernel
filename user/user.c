#include "user.h"

extern char __stack_top[];

void putchar(char c) {
    syscall(SYS_PUTCHAR, c, 0, 0);
}

int getchar(void) {
    return syscall(SYS_GETCHAR, 0, 0, 0);
}

__attribute__((section(".text.start"))) __attribute__((naked)) void start(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"
        "call main\n"
        "call exit\n" ::[stack_top] "r"(__stack_top));
}

__attribute__((noreturn)) void exit(void) {
    syscall(SYS_EXIT, 0, 0, 0);
    for (;;)
        ;  // unreachable but just in case
}

/**
 * @brief Executes a system call with the given system call number and arguments.
 *
 * @param sysno The system call number.
 * @param arg0 The first argument.
 * @param arg1 The second argument.
 * @param arg2 The third argument.
 * @return int The return value of the system call.
 *
 * @details The syscall function executes the ecall instruction with the system call number set in a3 and the system call
 * arguments in the a0 to a2 registers. ecall is a special instruction to delegate processing to the kernel. The return value from
 * the kernel is set in the a0 register.
 *
 */
int syscall(int sysno, int arg0, int arg1, int arg2) {
    register int a0 __asm__("a0") = arg0;
    register int a1 __asm__("a1") = arg1;
    register int a2 __asm__("a2") = arg2;
    register int a3 __asm__("a3") = sysno;

    __asm__ __volatile__("ecall" : "=r"(a0) : "r"(a0), "r"(a1), "r"(a2), "r"(a3) : "memory");

    return a0;
}