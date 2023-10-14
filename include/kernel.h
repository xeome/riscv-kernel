#pragma once

#include "common.h"

#define SATP_SV32 (1u << 31)   // Enable Sv32 mode
#define PAGE_V (1 << 0)        // Enable bit
#define PAGE_R (1 << 1)        // Read bit
#define PAGE_W (1 << 2)        // Write bit
#define PAGE_X (1 << 3)        // Execute bit
#define PAGE_U (1 << 4)        // User bit
#define PROCS_MAX 8            // Maximum number of processes
#define PROC_UNUSED 0          // Process is not in use
#define PROC_RUNNABLE 1        // Process is runnable
#define PROC_EXITED 2          // Process has exited
#define USER_BASE 0x1000000    // Base address of user memory
#define SSTATUS_SPIE (1 << 5)  // Supervisor Previous Interrupt Enable
#define SCAUSE_ECALL 8         // Environment call from U-mode

struct sbiret {
    long error;
    long value;
};

// Display the error and halt. Its a macro so that we can get the file and line number of the error correctly.
#define PANIC(fmt, ...)                                                       \
    do {                                                                      \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        while (1) {                                                           \
        }                                                                     \
    } while (0)

struct trap_frame {
    uint32_t ra;
    uint32_t gp;
    uint32_t tp;
    uint32_t t0;
    uint32_t t1;
    uint32_t t2;
    uint32_t t3;
    uint32_t t4;
    uint32_t t5;
    uint32_t t6;
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t a7;
    uint32_t s0;
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
    uint32_t sp;
} __attribute__((packed));

#define READ_CSR(reg)                                         \
    ({                                                        \
        unsigned long __tmp;                                  \
        __asm__ __volatile__("csrr %0, " #reg : "=r"(__tmp)); \
        __tmp;                                                \
    })

#define WRITE_CSR(reg, value)                                   \
    do {                                                        \
        uint32_t __tmp = (value);                               \
        __asm__ __volatile__("csrw " #reg ", %0" ::"r"(__tmp)); \
    } while (0)

struct process {
    int pid;               // Process ID
    int state;             // Process state
    vaddr_t sp;            // Stack pointer
    uint32_t* page_table;  // Page table
    uint8_t stack[8192];   // 8KB stack
};

// Process management

__attribute__((naked)) void switch_context(uint32_t* prev_sp, uint32_t* next_sp);
struct process* create_process(const void* image, size_t image_size);
void handle_trap(struct trap_frame* f);
void trap_handler(struct trap_frame* tf);
void yield(void);
void handle_syscall(struct trap_frame* f);

// Memory management

paddr_t alloc_pages(size_t n);
void free_pages(size_t n);
void map_page(uint32_t* page_table, vaddr_t va, paddr_t pa, uint32_t flags);

// Misc

__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void);
__attribute__((naked)) __attribute__((aligned(4))) void kernel_entry(void);
__attribute__((naked)) void user_entry(void);
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid);
void putchar(char ch);
void kernel_main(void);
long getchar(void);