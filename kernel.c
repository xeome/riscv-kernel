#include "kernel.h"
#include "common.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

// Declare symbols from linker script
extern char __bss[], __bss_end[], __stack_top[];
extern char __free_ram[], __free_ram_end[];
extern char __kernel_base[];

struct process procs[PROCS_MAX];
struct process* current_proc;  // Pointer to the currently running process
struct process* idle_proc;     // Pointer to the idle process

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
 * Creates a new process with the given program counter (pc).
 *
 * @param pc The program counter for the new process.
 * @return A pointer to the newly created process.
 * @throws PANIC if there are no free process slots.
 */
struct process* create_process(uint32_t pc) {
    // Find a free process slot
    struct process* proc = NULL;
    int i;
    for (i = 0; i < PROCS_MAX; i++) {
        if (procs[i].state == PROC_UNUSED) {
            proc = &procs[i];
            break;
        }
    }

    if (!proc)
        PANIC("no free process slots");

    // load the stack with call destination save registers so that switch_context() can return
    uint32_t* sp = (uint32_t*)&proc->stack[sizeof(proc->stack)];
    *--sp = 0;             // s11
    *--sp = 0;             // s10
    *--sp = 0;             // s9
    *--sp = 0;             // s8
    *--sp = 0;             // s7
    *--sp = 0;             // s6
    *--sp = 0;             // s5
    *--sp = 0;             // s4
    *--sp = 0;             // s3
    *--sp = 0;             // s2
    *--sp = 0;             // s1
    *--sp = 0;             // s0
    *--sp = (uint32_t)pc;  // ra

    uint32_t* page_table = (uint32_t*)alloc_pages(1);

    // Map the kernel memory
    for (paddr_t paddr = (paddr_t)__kernel_base; paddr < (paddr_t)__free_ram_end; paddr += PAGE_SIZE)
        map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);

    // Initialize the process structure
    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint32_t)sp;
    proc->page_table = page_table;
    return proc;
}

/**
 * Allocates n pages of memory and returns the physical address of the first page.
 *
 * @param n The number of pages to allocate.
 * @return The physical address of the first page.
 * @throws PANIC if there is not enough memory available.
 */
paddr_t alloc_pages(uint32_t n) {
    static paddr_t next_paddr = (paddr_t)__free_ram;
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    if (next_paddr > (paddr_t)__free_ram_end)
        PANIC("out of memory");

    memset((void*)paddr, 0, n * PAGE_SIZE);  // Clear the allocated memory
    return paddr;
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

/**
 * Writes a character to the console.
 *
 * @param ch The character to write.
 */
void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar */);
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
 * Handles a trap by printing an error message with the trap cause, trap value, and user program counter.
 * @param f A pointer to the trap frame.
 */
void handle_trap(struct trap_frame* f) {
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);  // user program counter

    PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
}

struct process* proc_a;
struct process* proc_b;

void proc_a_entry(void) {
    printf("starting process A\n");
    while (1) {
        putchar('A');
        yield();

        for (int i = 0; i < 30000000; i++)
            __asm__ __volatile__("nop");
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (1) {
        putchar('B');
        yield();

        for (int i = 0; i < 30000000; i++)
            __asm__ __volatile__("nop");
    }
}

// Kernel entry point function
void kernel_main(void) {
    memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);

    printf("\n\n");

    WRITE_CSR(stvec, (uint32_t)kernel_entry);

    idle_proc = create_process((uint32_t)NULL);
    idle_proc->pid = -1;  // idle
    current_proc = idle_proc;

    proc_a = create_process((uint32_t)proc_a_entry);
    proc_b = create_process((uint32_t)proc_b_entry);

    yield();
    PANIC("switched to idle\n");
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
 * This function yields the CPU to the next runnable process. It searches for the next runnable process
 * and if found, saves the current stack pointer, switches the context to the next process and restores
 * the stack pointer of the next process. If there are no other runnable processes, it continues running
 * the current process.
 *
 * @return void
 */
void yield(void) {
    // Find the next runnable process
    struct process* next = idle_proc;
    for (int i = 0; i < PROCS_MAX; i++) {
        struct process* proc = &procs[(current_proc->pid + i) % PROCS_MAX];
        if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
            next = proc;
            break;
        }
    }

    // There are no other runnable processes, so continue running the current process
    if (next == current_proc)
        return;

    // Context switch to the next process
    struct process* prev = current_proc;
    current_proc = next;

    // Save the current stack pointer and set the stack pointer to the top of the stack of the next process and switch the page
    // table
    __asm__ __volatile__(
        "sfence.vma\n"
        "csrw satp, %[satp]\n"
        "sfence.vma\n"
        "csrw sscratch, %[sscratch]\n"
        :
        : [satp] "r"(SATP_SV32 | ((uint32_t)next->page_table / PAGE_SIZE)), [sscratch] "r"(
                                                                                (uint32_t)&next->stack[sizeof(next->stack)]));

    switch_context(&prev->sp, &next->sp);
}

/**
 * Maps a physical page to a virtual address in the kernel page table.
 *
 * @param table1 Pointer to the first level page table.
 * @param vaddr Virtual address to map the physical page to.
 * @param paddr Physical address of the page to map.
 * @param flags Flags to set for the page table entry.
 */
void map_page(uint32_t* table1, uint32_t vaddr, paddr_t paddr, uint32_t flags) {
    if (!is_aligned(vaddr, PAGE_SIZE))
        PANIC("unaligned vaddr %x", vaddr);

    if (!is_aligned(paddr, PAGE_SIZE))
        PANIC("unaligned paddr %x", paddr);

    uint32_t vpn1 = (vaddr >> 22) & 0x3ff;  // shift 22 bits to right and mask 10 bits to extract vpn1 (First 10 bits)
    if ((table1[vpn1] & PAGE_V) == 0) {
        // Create a second level page table
        uint32_t pt_paddr = alloc_pages(1);                      // Allocate a physical page for the second level page table
        table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;  // Set the PPN (Page Physical Number) and V (Valid) bit
    }

    // Add an entry to the second level page table
    uint32_t vpn0 = (vaddr >> 12) & 0x3ff;  // shift 12 bits to right and mask 10 bits to extract vpn0 (Next 10 bits)
    uint32_t* table0 = (uint32_t*)((table1[vpn1] >> 10) * PAGE_SIZE);  // Get the physical address of the second level page table
    table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;       // Set the PPN (Page Physical Number) and V (Valid) bit
}