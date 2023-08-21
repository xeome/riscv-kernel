#pragma once

#include "common.h"

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

#define PROCS_MAX 8      // Maximum number of processes
#define PROC_UNUSED 0    // Process is not in use
#define PROC_RUNNABLE 1  // Process is runnable

struct process {
    int pid;              // Process ID
    int state;            // Process state
    vaddr_t sp;           // Stack pointer
    uint8_t stack[8192];  // 8KB stack
};

void kernel_main(void);
void yield(void);
void switch_context(uint32_t* prev_sp, uint32_t* next_sp);
void trap_handler(struct trap_frame* tf);
