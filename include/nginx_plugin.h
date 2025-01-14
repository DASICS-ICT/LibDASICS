#ifndef _INCLUDE_NGINX_PLUGIN_H
#define _INCLUDE_NGINX_PLUGIN_H
#include <ctype.h>
#include <stdint.h>
#include <udasics.h>

#define MB 0x100000UL
struct bound_t
{
    uint64_t lo;
    uint64_t hi;
    uint64_t flags;
};

struct openssl_elf_area
{
    uint64_t text_begin, text_end;
    uint64_t rw_num;
    struct bound_t rw_bound[DASICS_LIBCFG_WIDTH];
};

extern struct openssl_elf_area openssl_area;

void reloc_openssl();
void collect_openssl();
void init_openssl(uint64_t size);
void * openssl_malloc(uint64_t size);
void openssl_free(void * ptr);
void * openssl_realloc(void * ptr, uint64_t size);
void * openssl_calloc(uint64_t size);

#endif