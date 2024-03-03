#ifndef _INCLUDE_DASICS_STRING_H
#define _INCLUDE_DASICS_STRING_H
#include <stdint.h>
#include <syscall.h>

/* Rounding; only works for n = power of two */
#define ROUND(a, n)     (((((uint64_t)(a))+(n)-1)) & ~((n)-1))

#define ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))

extern void * dasics_memcpy(void *, const void *, uint64_t n);
extern void * dasics_memset(void *, int, uint64_t);
extern int dasics_strncmp(const char *cs, const char *ct, uint64_t count);
extern int dasics_strlen(const char *s);
extern int dasics_strncmp(const char *cs, const char *ct, uint64_t count);

static char *dasics_strcpy(char *dest, const char *src)
{
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
		/* nothing */;
	return tmp;
}

/**
 * dasics_strcmp - Compare two strings
 * @cs: One string
 * @ct: Another string
 */
static int dasics_strcmp(const char *cs, const char *ct)
{
	unsigned char c1, c2;

	while (1) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
	}
	return 0;
}

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

static inline void * dasics_malloc(uint64_t size)
{
    uint64_t heap = __BRK((uint64_t)ROUND(__BRK(0), 0x16UL));

    // brk syscall fail
    if (heap == 0) return (void *)0;

    uint64_t error = __BRK(heap + ROUND(size, 0x16UL));

    // brk syscall fail
    if (error == 0) return (void *)0;
   

    return (void *)heap;

}

#endif