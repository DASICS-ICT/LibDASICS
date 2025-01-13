#include <stdlib.h>
#include <stdint.h>
#include <ucsr.h>
#include <assert.h>
#include <sys/mman.h>
#include <dasics_stdio.h>
#include <dasics_string.h>
#include <openssl/ssl.h>
#include <dynamic.h>
#include <udasics.h>
#include <nginx_plugin.h>

struct openssl_elf_area openssl_area = {0};

void * openssl_self_heap = NULL;
uint64_t openssl_malloc_size = 0;
uint64_t openssl_full_size = 0;

void reloc_openssl()
{
    umain_elf_t * openssl = _get_area_by_name("libssl.so.3");
    assert(openssl != NULL);
    umain_elf_t * libcrypto = _get_area_by_name("libcrypto.so.3");
    assert(libcrypto != NULL);

    for (int i = 2; i < openssl->got_num + 2; i++)
    {
        if (openssl->target_elf[i] == libcrypto)
        {
            openssl->got_begin[i] = (uint64_t)openssl->_local_got_table[i];
        }
    }

    for (int i = 2; i < libcrypto->got_num + 2; i++)
    {
        if (libcrypto->target_elf[i] == openssl)
        {
            libcrypto->got_begin[i] = (uint64_t)libcrypto->_local_got_table[i];
        }
    }

    // Reloc malloc, free, realloc
    for (int i = 2; i < libcrypto->got_num + 2; i++)
    {
        if (!dasics_strcmp(_get_lib_name(libcrypto, i - 2), "malloc"))
        {
            libcrypto->_local_got_table[i] = (uint64_t)openssl_malloc;
        }
        if (!dasics_strcmp(_get_lib_name(libcrypto, i - 2), "free"))
        {
            libcrypto->_local_got_table[i] = (uint64_t)openssl_free;
        }
        if (!dasics_strcmp(_get_lib_name(libcrypto, i - 2), "realloc"))
        {
            libcrypto->_local_got_table[i] = (uint64_t)openssl_realloc;
        }
    }
    
}

void collect_openssl()
{
    umain_elf_t * openssl = _get_area_by_name("libssl.so.3");
    assert(openssl != NULL);
    umain_elf_t * libcrypto = _get_area_by_name("libcrypto.so.3");
    assert(libcrypto != NULL);
    umain_elf_t * libc = _get_area_by_name("libc.so.6");
    assert(libc != NULL);    
    // text
    if (openssl->_text_start < libcrypto->_text_end)
    {
        openssl_area.text_begin = openssl->_text_start;
        openssl_area.text_end = libcrypto->_text_end;
    }
    else
    {
        openssl_area.text_begin = libcrypto->_text_start;
        openssl_area.text_end = openssl->_text_end;
    }
    int idx_rw=0;
    register uint64_t tp asm ("tp");
    // rw_start
    openssl_area.rw_bound[idx_rw].lo = openssl->_r_start;
    openssl_area.rw_bound[idx_rw].hi = openssl->_w_end;
    idx_rw++;
    openssl_area.rw_bound[idx_rw].lo = libcrypto->_r_start;
    openssl_area.rw_bound[idx_rw].hi = libcrypto->_w_end;
    idx_rw++;
    openssl_area.rw_bound[idx_rw].lo = libc->_r_start;
    openssl_area.rw_bound[idx_rw].hi = libc->_w_end;
    idx_rw++;
    // tp
    openssl_area.rw_bound[idx_rw].lo = tp;
    openssl_area.rw_bound[idx_rw].hi = tp + PAGE_SIZE;
    idx_rw++;

    openssl_area.rw_num = idx_rw;
}

void init_openssl(uint64_t size)
{
    // First init memory
    void * self_heap = mmap(NULL, size, \
                            PROT_READ | PROT_WRITE, \
                            MAP_PRIVATE | MAP_ANONYMOUS, \
                            -1, \
                            0);

    if (self_heap == NULL || (uint64_t)self_heap < 0)
    {
        dasics_printf("Error: init_openssl_self_heap failed, hang\n");
        while(1);
    }
    openssl_full_size = size;
    openssl_self_heap = self_heap;

    dasics_printf("Set openssl self heap begin: 0x%lx, end: 0x%lx\n", (uint64_t)openssl_self_heap, (uint64_t)openssl_self_heap + openssl_full_size);

    // Reloc
    reloc_openssl();

    // Collect
    collect_openssl();


}


void * openssl_malloc(uint64_t size)
{
    if (openssl_malloc_size >= openssl_full_size)
    {
        dasics_printf("Error: openssl_malloc failed, hang\n");
        while(1);
    }
    void * ret = (void *)openssl_self_heap;
    openssl_self_heap =  (void *)((uint64_t)openssl_self_heap + ROUND(size, 16));
    openssl_malloc_size += ROUND(size, 16);
    return ret;
}

void openssl_free(void * ptr)
{
    // Do nothing
}

void * openssl_realloc(void * ptr, uint64_t size)
{
    void * ptr_new = openssl_malloc(size);
    dasics_memcpy(ptr_new, ptr, size);

    return ptr_new;
}