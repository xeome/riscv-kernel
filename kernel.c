#include "kernel.h"
#include "common.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

// Declare symbols from linker script
extern char __bss[], __bss_end[], __stack_top[];

// sbi_call is a wrapper around the ecall instruction
struct sbiret sbi_call(long arg0,
                       long arg1,
                       long arg2,
                       long arg3,
                       long arg4,
                       long arg5,
                       long fid,
                       long eid) {
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
                           "=r"(a1)  // Outputs: a0 will hold the error code, a1
                                     // will hold the value
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                           "r"(a6), "r"(a7)
                         : "memory");  // Clobbers: the ecall instruction may
                                       // modify any register or memory location
                                       // in the system
    return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar */);
}

// Function to fill a memory block with a specific value
void* memset(void* buf, char c, size_t n) {
    uint8_t* p = (uint8_t*)buf;

    // Loop to write the value 'c' to 'n' bytes in memory
    while (n--)
        *p++ = c;
    // Return the original memory address
    return buf;
}

// Kernel entry point function
void kernel_main(void) {
    printf("\n\nHello %s\n", "World!");
    printf("1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);

    for (;;) {
        // Wait for interrupt
        __asm__ __volatile__("wfi");
    }
}

// Boot entry point function, placed in .text.boot section
// The `attribute((naked))` directive tells the compiler not to generate any
// extra code around the body of the function. This allows the content of the
// function to be output exactly as written in inline assembly.
__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
    __asm__ __volatile__(
        // Set the stack pointer to the top of the stack
        "mv sp, %[stack_top]\n"
        // Jump to the kernel_main function
        "j kernel_main\n"
        :
        : [stack_top] "r"(
            __stack_top));  // Input constraint specifying %[stack_top]
                            // corresponds to __stack_top
}