typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

// Declare symbols from linker script
extern char __bss[], __bss_end[], __stack_top[];

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
    // Initialize the .bss section with zeros
    memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);

    // Halt
    for (;;)
        ;
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