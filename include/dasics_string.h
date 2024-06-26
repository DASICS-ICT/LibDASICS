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


#endif