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
#ifdef DASICS_NGINX_MODE1
#include <nginx_plugin.h>


uint64_t cross_stack = 0;
uint64_t cross_stack_base = 0;
#else
#include <dmalloc.h>

uint64_t cross_stack = 0;
uint64_t cross_stack_base = 0;
static uint64_t cross_stack_limit;
#endif

/* malloc cross stack used cross depth */ 
void init_cross_stack()
{
    int malloc_size = (sizeof(struct cross) + \
                        sizeof(int) * DASICS_LIBCFG_WIDTH) * MAX_DEPTH;
#ifdef DASICS_NGINX_MODE1
    cross_stack_base = (uint64_t)malloc(malloc_size);

    if(!cross_stack_base)
    {
        perror("DASICS: init cross_stack failed\n");
    }

    cross_stack = cross_stack_base + malloc_size;
#else
    cross_stack_base = (int64_t)_dasics_mmap(0, \
                            malloc_size, \
                            PROT_READ | PROT_WRITE, \
                            MAP_PRIVATE | MAP_ANONYMOUS, 
                            -1,
                            0);//(uint64_t)malloc(malloc_size);

    if (!cross_stack_base || cross_stack_base >= DASICS_USER_LIMIT ||
        (uint64_t)malloc_size > DASICS_USER_LIMIT - cross_stack_base)
    {
        perror("DASICS: init cross_stack failed");
        exit(1);
    }

    cross_stack_limit = cross_stack_base + malloc_size;
    cross_stack = cross_stack_limit;
#endif
}

/* Return a init handler */ 
void push_cross(struct cross * tmp)
{
    int stack_spend = sizeof(struct cross);

#ifdef DASICS_NGINX_MODE1
    if (stack_spend > (cross_stack - cross_stack_base))
        perror("DASICS: push cross failed\n");
#else
    if (!cross_stack_base || cross_stack < cross_stack_base || cross_stack > cross_stack_limit ||
        (uint64_t)stack_spend > cross_stack - cross_stack_base) exit(1);
#endif

    cross_stack -= stack_spend;
    
    struct cross * cross_handle =(struct cross *)cross_stack; 

    dasics_memcpy((void *)cross_stack, tmp, sizeof(struct cross));    
}


#ifdef DASICS_NGINX_MODE1
struct cross * push_cross_get()
{
    int stack_spend = sizeof(struct cross);

    if (stack_spend > (cross_stack - cross_stack_base))
        perror("DASICS: push cross failed\n");

    cross_stack -= stack_spend;

    dasics_memset((struct cross *)cross_stack, 0, sizeof(struct cross));
    return (struct cross *)cross_stack;
}

/* Pop a cross stack */
void pop_cross(struct umaincall * maincallContext)
{
#else
/* Pop a cross stack */
void pop_cross(struct umaincall * maincallContext)
{
    if (!cross_stack_base || cross_stack < cross_stack_base ||
        cross_stack >= cross_stack_limit) exit(1);
#endif
    struct cross * cross_handle =(struct cross *)cross_stack; 

    // Realse Bounds
    for (int i = 0; i < cross_handle->handle_num; i++)
    {
#ifdef DASICS_NGINX_MODE1
        assert(dasics_libcfg_free(cross_handle->handle[i]) == 0);
    }
    maincallContext->ra = cross_handle->ra;

    // Free Jmp
    for (int i = 0; i < cross_handle->jmp_num; i++)
    {
        assert(dasics_jumpcfg_free(cross_handle->jmpcfg[i]) == 0);
    }

    if (cross_handle->clear_active)
    {
        openssl_area.is_active = 0;
        assert(dasics_libcfg_inactive(cross_handle->longTimeHandle, cross_handle->longTimeHandle_num) == 0);
    }

    // umain_elf_t * target = cross_handle->target;
    // umain_elf_t * entry = cross_handle->begin;

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
    //             if (bounds[i].addr) {
    //                 assert(dasics_libcfg_free(bounds[i].handler) != -1);
    //                 bounds[i].addr = 0;
    //             }

    //         }
    //     }
    // }
#else
        if (cross_handle->handle[i] >= 0)
            assert(dasics_libcfg_free(cross_handle->handle[i]) == 0);        
    }
    maincallContext->ra = cross_handle->ra;
    maincallContext->return_pc = cross_handle->return_pc;
    maincallContext->freezone_return_pc = cross_handle->freezone_return_pc;

    // Free Jmp
    for (int i = 0; i < DASICS_JUMPCFG_WIDTH; i++)
    {
        if (cross_handle->jmpcfg[i] >= 0)
            assert(dasics_jumpcfg_free(cross_handle->jmpcfg[i]) == 0);
    }
    
    umain_elf_t * target = cross_handle->target;
    umain_elf_t * entry = cross_handle->begin;

    struct func_mem * mem = NULL;

    // if entry is untrusted, update global_func_mem
    if (!(target->_flags & MAIN_AREA))
    {
        mem = target->namespace_func;
    }

    // if target is untrusted, cleart bounds
    if (!(target->_flags & MAIN_AREA))
    {
        if (mem)
        {
            // Alloc bound
            struct bound_table * bounds = mem->mem;


            for (int i = 0; i < mem->bound_max; i++)
            {
                /* code */
                if (bounds[i].addr)
                    assert(dasics_libcfg_free(bounds[i].handler) != -1);
            }        
        }
    }
#endif

    // Free stack area
    cross_stack += sizeof(struct cross);
}
