#ifndef _INCLUDE_DASICS_STRING_H
#define _INCLUDE_DASICS_STRING_H

static inline int __dasics_linker_memcpy(char *dest, const char *src, unsigned int len)
{
    for (int i = 0; i < len; i++)
    {
        dest[i] = src[i];
    }
    return len;
}

static inline char * __dasics_linker_strcpy(char *dest, const char *src)
{
    char *tmp = dest;

    while (*src) {
        *dest++ = *src++;
    }

    *dest = '\0';

    return tmp;
}

static inline int __dasics_linker_strcmp(const char *str1, const char *str2)
{
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            return (*str1) - (*str2);
        }
        ++str1;
        ++str2;
    }
    return (*str1) - (*str2);
}


#endif