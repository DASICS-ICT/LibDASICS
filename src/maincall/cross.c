#include <cross.h>
#include <stdlib.h>
#include <string.h>
#include <udasics.h>
#include <assert.h>
#include <umaincall.h>
#include <dynamic.h>
#include <dasics_start.h>
#include <dasics_string.h>
#include <ufuncmem.h>
#include <dmalloc.h>

uint64_t cross_stack = 0;
uint64_t cross_stack_base = 0;

/* malloc cross stack used cross depth */ 
void init_cross_stack()
{
    int malloc_size = (sizeof(struct cross) + \
                        sizeof(int) * DASICS_LIBCFG_WIDTH) * MAX_DEPTH;
    // cross_stack_base = (uint64_t)malloc(malloc_size);
    cross_stack_base = (int64_t)_dasics_mmap(0, \
                            malloc_size, \
                            PROT_READ | PROT_WRITE, \
                            MAP_PRIVATE | MAP_ANONYMOUS, \
                            -1, \
                            0);
    if(cross_stack_base <= 0)
    {
        perror("DASICS: init cross_stack failed\n");
    }

    cross_stack = cross_stack_base + malloc_size;
}

/* Return a init handler */ 
void push_cross(struct cross * tmp)
{
    int stack_spend = sizeof(struct cross);

    if (stack_spend > (cross_stack - cross_stack_base))
        perror("DASICS: push cross failed\n");

    cross_stack -= stack_spend;
    
    struct cross * cross_handle =(struct cross *)cross_stack; 

    dasics_memcpy((void *)cross_stack, tmp, sizeof(struct cross));    
}


/* Pop a cross stack */
void pop_cross(struct umaincall * maincallContext)
{
    struct cross * cross_handle =(struct cross *)cross_stack; 

    // Realse Bounds
    // for (int i = 0; i < cross_handle->handle_num; i++)
    // {
    //     if (cross_handle->handle[i])
    //         assert(dasics_libcfg_free(cross_handle->handle[i]) == 0);        
    // }
    maincallContext->ra = cross_handle->ra;

    // Free Jmp
    // for (int i = 0; i < DASICS_JUMPCFG_WIDTH; i++)
    // {
    //     if (cross_handle->jmpcfg[i])
    //         assert(dasics_jumpcfg_free(cross_handle->jmpcfg[i]) == 0);
    // }
    
    // umain_elf_t * target = cross_handle->target;
    // umain_elf_t * entry = cross_handle->begin;

    // dasics_printf("[RETURN]: RETURN from %s to %s\n", target->real_name, entry->real_name);
    // struct func_mem * mem = NULL;

    // // if entry is untrusted, update global_func_mem
    // if (!(target->_flags & MAIN_AREA))
    // {
    //     mem = target->namespace_func;
    // }

    // // if target is untrusted, cleart bounds
    // if (!(target->_flags & MAIN_AREA))
    // {
    //     if (mem)
    //     {
    //         // Alloc bound
    //         struct bound_table * bounds = mem->mem;


    //         for (int i = 0; i < mem->bound_max; i++)
    //         {
    //             /* code */
    //             if (bounds[i].addr)
    //                 assert(dasics_libcfg_free(bounds[i].handler) != -1);
    //         }        
    //     }
    // }

    // Free stack area
    cross_stack += sizeof(struct cross);
}
