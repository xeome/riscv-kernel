#pragma once

typedef int bool;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef uint32_t size_t;
// A physical address is an address that the memory management unit (MMU) uses to access main memory.
typedef uint32_t paddr_t;
// A virtual address is an address that is used by the CPU to identify a location in memory.
typedef uint32_t vaddr_t;

#define true 1
#define false 0
#define NULL ((void*)0)
// Round up to the nearest multiple of n (n must be a power of 2)
#define align_up(value, align) __builtin_align_up(value, align)
// Determine if the given value is aligned to the given alignment (n must be a power of 2)
#define is_aligned(value, align) __builtin_is_aligned(value, align)
// Return the offset of the given member within a struct (how many bytes from the beginning of the structure)
#define offsetof(type, member) __builtin_offsetof(type, member)

// va_list is a type that represents a variable argument list. It is used to declare a list of arguments whose number
// and types are not known at compile time.
#define va_list __builtin_va_list
// va_start is a macro that initializes a va_list object with the first argument after the format string. It takes two
// arguments: the first is the va_list object to be initialized, and the second is the name of the last fixed argument
// in the function (in this case, fmt).
#define va_start __builtin_va_start
// va_end is a macro that cleans up a va_list object after use. It takes one argument: the va_list object to be cleaned
// up.
#define va_end __builtin_va_end
// After va_arg is used to retrieve an argument from the variable argument list, the argument is removed from the list.
// This means that subsequent calls to va_arg will retrieve the next argument in the list.
#define va_arg __builtin_va_arg

void* memset(void* buf, char c, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
char* strcpy(char* dst, const char* src);
int strcmp(const char* s1, const char* s2);
void printf(const char* fmt, ...);