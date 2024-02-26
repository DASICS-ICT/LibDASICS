#ifndef _INCLUDE_DASICS_STRING_H
#define _INCLUDE_DASICS_STRING_H
#include <stdint.h>
#include <syscall.h>

extern void * dasics_memcpy(void *, const void *, uint64_t n);
extern void * dasics_memset(void *, int, uint64_t);
extern int dasics_strncmp(const char *cs, const char *ct, uint64_t count);
extern int dasics_strlen(const char *s);
extern int dasics_strncmp(const char *cs, const char *ct, uint64_t count);


// malloc memory on heap simplily
static inline uint64_t __BRK(uint64_t ptr)
{
    register uint64_t a7 asm("a7") = __NR_brk;
    register uint64_t a0 asm("a0") = ptr;
    asm volatile("ecall"                        \
                 : "+r"(a0)                     \
                 : "r"(a7)                      \
                 : "memory");

    return a0;
}

#endif