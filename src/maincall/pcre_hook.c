#include <stdlib.h>
#include <stdint.h>
#include <ucsr.h>
#include <assert.h>
#include <sys/mman.h>
#include <dasics_stdio.h>
#include <dasics_string.h>
#include <dynamic.h>
#include <udasics.h>
#include <nginx_plugin.h>

struct pcre_heap pcre_heap_info;

void * pcre_self_heap = NULL;
uint64_t pcre_malloc_size = 0;
uint64_t pcre_full_size = 0;

static void* pcre_malloc(uint64_t size) {
    if (pcre_malloc_size >= pcre_full_size) {
        dasics_printf("Error: pcre_malloc failed, hang\n");
        while(1);
    }
    // dasics_printf("[LOG]: pcre malloc base: 0x%lx, size: 0x%lx\n", (uint64_t)pcre_self_heap, ROUND(size, 16));
    void *ret = (void *)pcre_self_heap;
    pcre_self_heap = (void *)((uint64_t)pcre_self_heap + ROUND(size, 16));
    pcre_malloc_size += ROUND(size, 16);

    return ret;
}

static void pcre_free(void *ptr) {
    // do nothing
}

static void reloc_pcre() {
    umain_elf_t *pcre = _get_area_by_name("libpcre.so.1");
    assert(pcre != NULL);

    // reloc malloc, free
    for (int i = 2; i < pcre->got_num + 2; i++) {
        char *name = pcre->target_func_name[i];
        if (!dasics_strcmp(name, "malloc")) {
            pcre->_local_got_table[i] = (uint64_t)pcre_malloc;
        }
        if (!dasics_strcmp(name, "free")) {
            pcre->_local_got_table[i] = (uint64_t)pcre_free;
        }
    }
}

void init_pcre(uint64_t size) {
    void * self_heap = mmap(NULL, size, \
        PROT_READ | PROT_WRITE, \
        MAP_PRIVATE | MAP_ANONYMOUS, \
        -1, \
        0);

    if (self_heap == NULL || (uint64_t)self_heap < 0) {
        dasics_printf("Error: init_pcre_self_heap failed, hang\n");
        while(1);
    }

    pcre_full_size = size;
    pcre_self_heap = self_heap;

    pcre_heap_info.base = pcre_self_heap;
    pcre_heap_info.size = size;

    dasics_printf("Set pcre self heap begin: 0x%lx, end: 0x%lx\n", 
        (uint64_t)pcre_self_heap, (uint64_t)pcre_self_heap + pcre_full_size);

    // reloc
    reloc_pcre();
}
