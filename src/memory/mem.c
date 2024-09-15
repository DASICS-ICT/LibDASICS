#include <mem.h>
#include <openssl_mem.h>
#include <stdlib.h>
#include <stdint.h>
#include <umaincall.h>
#include <ucsr.h>
#include <assert.h>
#include <sys/mman.h>
#include <dasics_stdio.h>
#include <dasics_string.h>
#include <openssl/ssl.h>

void * openssl_self_heap = NULL;
uint64_t openssl_malloc_size = 0;
uint64_t openssl_full_size = 0;

void init_openssl_self_heap(uint64_t size)
{
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
    update_self_heap_metadata(self_heap, size);

}

int dasics_openssl_umaincall_hook(struct umaincall * regs)
{
    assert(openssl_self_heap != NULL);
    uint64_t dasics_return_pc = csr_read(0x8b4);            // DasicsReturnPC
    // uint64_t dasics_free_zone_return_pc = csr_read(0x8b2);  // DasicsFreeZoneReturnPC
    if (regs->a0 == DASICS_HOOK_MAGIC)
    {
        regs->a0 = 0;
        switch (regs->a1)
        {
        case DASICS_MALLOC_HOOK:
        {

            // regs->a0 = (uint64_t)malloc(regs->a2);
            // regs->a0 = (uint64_t)openssl_self_heap
            assert(openssl_malloc_size < openssl_full_size);
            uint64_t p = ROUND(regs->a2, 8);
            regs->a0 = (uint64_t)openssl_self_heap;
            // dasics_printf("Malloc: malloc %lx\n",  regs->a0);
            openssl_self_heap = (void *)((uint64_t)openssl_self_heap + p);
            openssl_malloc_size += p;
            update_self_heap_used(openssl_malloc_size);
            break;
        }
        case DASICS_FREE_HOOK:
        {
            // Do nothing
            // free((void *)regs->a2);
            break;
        }
            
        case DASICS_REALLOC_HOOK:
        {
            // regs->a0 = (uint64_t)realloc((void *)regs->a2, regs->a3);
            assert(openssl_malloc_size < openssl_full_size);
            uint64_t p = ROUND(regs->a3, 8);
            regs->a0 = (uint64_t)openssl_self_heap;
            dasics_memcpy(openssl_self_heap, (void *)regs->a2, regs->a3);
            // dasics_printf("Malloc: malloc %lx\n",  regs->a0);
            openssl_self_heap = (void *)((uint64_t)openssl_self_heap + p);
            openssl_malloc_size += p;
            update_self_heap_used(openssl_malloc_size);
            break;
        }
        default:
            assert(0);
            break;
        }
        regs->t1 = regs->ra;
        csr_write(0x8b4, dasics_return_pc);             // DasicsReturnPC
        asm("fence.i");
        // csr_write(0x8b2, dasics_free_zone_return_pc);   // DasicsFreeZoneReturnPC        
        return 1;
    }

    return 0;
}


void * umain_malloc_hook(size_t size)
{
    dasics_printf("[LOG]: umain_malloc_hook\n");
    return udasics_malloc_handler(size);
}


void * umain_realloc_hook(void * addr, size_t new_size)
{
    dasics_printf("[LOG]: umain_reaalloc_hook\n");
    return udasics_realloc_handler(addr, new_size);
}

void umain_free_hook(void * addr)
{
    dasics_printf("[LOG]: umain_free_hook\n");
    udasics_free_handler(addr);
}