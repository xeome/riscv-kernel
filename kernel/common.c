#include "common.h"

void putchar(char ch);

/**
 * @brief This function implements a basic version of printf that supports the following format specifiers:
 *        %s - string
 *        %d - decimal integer
 *        %x - hexadecimal integer
 *
 * @param fmt Format string
 * @param ... Variable arguments
 */
void printf(const char* fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case '\0':
                case '%':
                    putchar('%');
                    break;
                case 's': {
                    const char* s = va_arg(vargs, const char*);
                    while (*s) {
                        putchar(*s);
                        s++;
                    }
                    break;
                }
                case 'd': {
                    int value = va_arg(vargs, int);
                    if (value < 0) {
                        putchar('-');
                        value = -value;
                    }

                    int divisor = 1;
                    while (value / divisor > 9)
                        divisor *= 10;

                    while (divisor > 0) {
                        putchar('0' + value / divisor);
                        value %= divisor;
                        divisor /= 10;
                    }

                    break;
                }
                case 'p':
                case 'X':
                case 'x': {
                    const int value = va_arg(vargs, int);
                    for (int i = 7; i >= 0; i--) {
                        const int nibble = (value >> (i * 4)) & 0xf;
                        putchar("0123456789abcdef"[nibble]);
                    }
                }
                default: {
                    // Unsupported format specifier. Just print the specifier itself.
                    putchar('%');
                    putchar(*fmt);
                    break;
                }
            }
        } else {
            putchar(*fmt);
        }

        fmt++;
    }

    va_end(vargs);
}

/**
 * Sets the first 'n' bytes of the memory area pointed to by 'buf' to the specified value 'c'.
 *
 * @param buf Pointer to the memory area to be filled.
 * @param c Value to be set. The value is passed as an int, but the function fills the memory using the unsigned char
 * conversion of this value.
 * @param n Number of bytes to be set to the value.
 *
 * @return A pointer to the memory area 'buf'.
 */
void* memset(void* buf, char c, size_t n) {
    uint8_t* p = (uint8_t*)buf;

    // Loop to write the value 'c' to 'n' bytes in memory
    while (n--)
        *p++ = c;  // Write 'c' to the current address and increment it by 1
    // Return the original memory address
    return buf;
}

/**
 * Copies n bytes from memory area src to memory area dst.
 *
 * @param dst Pointer to the destination array where the content is to be copied.
 * @param src Pointer to the source of data to be copied.
 * @param n Number of bytes to copy.
 *
 * @return A pointer to the destination array, which is dst.
 */
void* memcpy(void* dst, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    while (n--)
        *d++ = *s++;  // Copy the current byte and increment the pointers
    return dst;
}

/**
 * Copies the string pointed to by src, including the null character, to the buffer pointed to by dst.
 *
 * @param dst The destination buffer to copy the string to.
 * @param src The source string to copy from.
 * @return A pointer to the destination buffer.
 */
char* strcpy(char* dst, const char* src) {
    char* d = dst;
    while (*src)
        *d++ = *src++;
    *d = '\0';
    return dst;
}
/**
 * Compares two strings.
 *
 * @param s1 The first string to compare.
 * @param s2 The second string to compare.
 *
 * @return An integer less than, equal to, or greater than zero if s1 is found, respectively, to be less than, to match,
 * or be greater than s2.
 */
int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2)
            break;
        s1++;
        s2++;
    }

    return *s1 - *s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (*s1 && *s2 && n) {
        if (*s1 != *s2)
            break;
        s1++;
        s2++;
        n--;
    }

    return *s1 - *s2;
}

int strlen(const char* s) {
    int len = 0;
    while (*s++)
        len++;
    return len;
}