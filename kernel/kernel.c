#include "kernel.h"
#include "common.h"
#include "virtio.h"
#include "tarfs.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

// Declare symbols from linker script
extern char __bss[], __bss_end[], __stack_top[];
extern char __free_ram[], __free_ram_end[];
extern char __kernel_base[];
// Use names from `llvm-nm build/shell.bin.o` command
extern char _binary_build_shell_bin_start[], _binary_build_shell_bin_size[];

struct process procs[PROCS_MAX];
struct process* current_proc;  // Pointer to the currently running process
struct process* idle_proc;     // Pointer to the idle process

// Kernel entry point function
void kernel_main(void) {
    memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);
    WRITE_CSR(stvec, (uint32_t)kernel_entry);

    paddr_t test = alloc_pages(1);
    free_pages(1);
    paddr_t test2 = alloc_pages(1);
    if (test != test2)
        PANIC("free_pages() failed\n");

    virtio_blk_init();
    fs_init();

    idle_proc = create_process(NULL, 0);
    idle_proc->pid = -1;  // idle
    current_proc = idle_proc;

    create_process(_binary_build_shell_bin_start, (size_t)_binary_build_shell_bin_size);

    yield();
    PANIC("switched to idle\n");
}

/**
 * Creates a new process with the given image and image size.
 *
 * @param image A pointer to the image of the process.
 * @param image_size The size of the image in bytes.
 *
 * @return A pointer to the newly created process structure.
 *
 * @details This function finds a free process slot, loads the stack with call destination save registers so that switch_context()
 * can return, maps the kernel memory, maps the user memory, initializes the process structure and returns a pointer to the newly
 * created process structure.
 */
struct process* create_process(const void* image, size_t image_size) {
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
    *--sp = 0;                     // s11
    *--sp = 0;                     // s10
    *--sp = 0;                     // s9
    *--sp = 0;                     // s8
    *--sp = 0;                     // s7
    *--sp = 0;                     // s6
    *--sp = 0;                     // s5
    *--sp = 0;                     // s4
    *--sp = 0;                     // s3
    *--sp = 0;                     // s2
    *--sp = 0;                     // s1
    *--sp = 0;                     // s0
    *--sp = (uint32_t)user_entry;  // ra

    uint32_t* page_table = (uint32_t*)alloc_pages(1);

    // Map the kernel memory
    for (paddr_t paddr = (paddr_t)__kernel_base; paddr < (paddr_t)__free_ram_end; paddr += PAGE_SIZE)
        map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);

    // VirtIO-blk
    map_page(page_table, VIRTIO_BLK_PADDR, VIRTIO_BLK_PADDR, PAGE_R | PAGE_W);

    // Map the user memory
    for (uint32_t off = 0; off < image_size; off += PAGE_SIZE) {
        paddr_t page = alloc_pages(1);
        memcpy((void*)page, image + off, PAGE_SIZE);
        map_page(page_table, USER_BASE + off, page, PAGE_U | PAGE_R | PAGE_W | PAGE_X);  // PAGE_U: user mode accessible
    }

    // Initialize the process structure
    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint32_t)sp;
    proc->page_table = page_table;
    return proc;
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
 * Handles system calls based on the value of a3 in the trap frame.
 * if a3 is not a valid system call number, the kernel panics.
 *
 * @param f Pointer to the trap frame containing the system call information.
 */
void handle_syscall(struct trap_frame* f) {
    switch (f->a3) {
        case SYS_PUTCHAR:
            putchar(f->a0);  // a0 contains the character to write
            break;
        case SYS_GETCHAR:
            while (1) {
                long ch = getchar();
                if (ch >= 0) {
                    f->a0 = ch;
                    break;
                }

                yield();  // Yield the CPU to allow other processes to run
            }
            break;
        case SYS_EXIT:
            printf("process %d exited\n", current_proc->pid);
            current_proc->state = PROC_EXITED;
            yield();
            PANIC("unreachable");
        case SYS_READFILE:
        case SYS_WRITEFILE: {
            // a0 contains the filename, a1 contains the buffer, a2 contains the length
            const char* filename = (const char*)f->a0;
            char* buf = (char*)f->a1;
            int len = f->a2;
            // Look up the file
            struct file* file = fs_lookup(filename);
            if (!file) {
                printf("file not found: %s\n", filename);
                f->a0 = -1;
                break;
            }

            // Truncate the length if it is larger than the file size
            if (len > (int)sizeof(file->data))
                len = file->size;

            // Read or write the file
            if (f->a3 == SYS_WRITEFILE) {
                memcpy(file->data, buf, len);
                file->size = len;
                fs_flush();
            } else {
                memcpy(buf, file->data, len);
            }

            f->a0 = len;
            break;
        }
        default:
            PANIC("unexpected syscall a3=%x\n", f->a3);
    }
}

/**
 * Handles a trap by printing an error message with the trap cause, trap value, and user program counter.
 * @param f A pointer to the trap frame.
 */
void handle_trap(struct trap_frame* f) {
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);  // user program counter
    if (scause == SCAUSE_ECALL) {
        handle_syscall(f);
        user_pc += 4;  // skip ecall instruction
        WRITE_CSR(sepc, user_pc);
        return;
    }

    PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
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

long getchar(void) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
    return ret.error;
}