#include <ufuncmem.h>
#include <utrap.h>
#include <umaincall.h>
#include <udasics.h>
#include <dynamic.h>
#include <dasics_start.h>
#include <dasics_string.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct func_mem * global_func_mem = NULL;

// Malloc a init func mem
static struct func_mem * init_func_mem(uint64_t func)
{
    struct func_mem * tmp = (struct func_mem *)malloc(sizeof(struct func_mem));
    assert(tmp != NULL);
    
    // Malloc bound
    tmp->mem = (struct bound_table *)calloc(MAX_BOUNS, sizeof(struct bound_table));
    assert(tmp->mem != NULL);
    tmp->func = func;
    tmp->bound_max = MAX_BOUNS;

    tmp->bound_num = 0;

    init_list(&tmp->list);

    return tmp;
}

static struct bound_table * _expan_bound(struct func_mem * tmp)
{
    // Expand more MAX_BOUNDS bound table
    tmp->mem = realloc(tmp->mem, sizeof(struct bound_table) * tmp->bound_max + EXPAN_BOUNDS);
    assert(tmp->mem != NULL);

    // Clear the expand mem
    memset((void *)(&tmp->mem[tmp->bound_max]), 0, EXPAN_BOUNDS);
    tmp->bound_max += MAX_BOUNS;

    return tmp->mem;
}

// Set and found a func mem
int set_global_func_man(umain_elf_t *entry, uint64_t func)
{
    struct func_mem * find = NULL, *find_t = NULL;

    struct func_mem *result = NULL; 
    list_for_each_entry_safe(find, find_t, &entry->func_man, list)
    {
        if (find->func == func)
        {
            result = find;
            break;
        }
    }

    if (result == NULL) result = init_func_mem(func);
    list_add_tail(&result->list, &entry->func_man);

    global_func_mem = result;
    return 0;

}


// Handle mem
void * handle_lib_malloc(struct umaincall * callContext)
{
    assert(global_func_mem != NULL);

    struct func_mem * tmp = global_func_mem;

    // Expand
    if (tmp->bound_num == tmp->bound_max)
        _expan_bound(tmp);

    void * malloc_mem = malloc(callContext->a0);
    assert(malloc_mem != NULL);

    struct bound_table * record = NULL;
    // Find a place to set
    for (int i = 0; i < tmp->bound_max; i++)
    {

        if (tmp->mem[i].addr == 0)
        {
            tmp->mem[i].addr = (uint64_t)malloc_mem;
            tmp->mem[i].length = callContext->a0;
            record = &tmp->mem[i];
            break;
        }
    }
    tmp->bound_num++;
    // Return result to caller
    callContext->a0 = (uint64_t)malloc_mem;


    int32_t handler = dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_W | DASICS_LIBCFG_V, \
                        record->addr, record->addr + record->length);

    record->handler = handler;

    return 0;
}

// Handle realloc
int handle_lib_realloc(struct umaincall * callContext)
{
    assert(global_func_mem != NULL);

    struct func_mem * tmp = global_func_mem;
    struct bound_table * bounds = global_func_mem->mem;

    int realloc_idx = -1;

    struct bound_table * target = NULL;
    for (int i = 0; i < tmp->bound_max; i++)
    {
        if (bounds[i].addr == callContext->a0)
        {
            target = &bounds[i];
            break;
        }
    }
    
    void * new_addr = realloc((void *)callContext->a0, callContext->a1);
    assert(new_addr != NULL);

    target->addr = (uint64_t)new_addr;
    target->length = callContext->a1;

    dasics_libcfg_free(target->handler);

    target->handler =  dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_W | DASICS_LIBCFG_V, \
                        target->addr, target->addr + target->length);

    // Return result to caller
    callContext->a0 = (uint64_t)new_addr;

    

    return 0;
}


// Handle free
int handle_lib_free(struct umaincall * callContext)
{
    assert(global_func_mem != NULL);

    struct func_mem * tmp = global_func_mem;
    struct bound_table * bounds = global_func_mem->mem;


    struct bound_table * target = NULL;
    for (int i = 0; i < tmp->bound_max; i++)
    {
        if (bounds[i].addr == callContext->a0)
        {
            target = &bounds[i];
            break;
        }
    }

    assert(target != NULL);    

    free((void *)callContext->a0);

    tmp->bound_num--;

    target->addr = 0;
    target->length = 0;

    // Return result to caller
    callContext->a0 = 0;    
    
    dasics_libcfg_free(target->handler);

    target->handler = -1;

    return 0;
}


int handle_lib_mem(umain_elf_t *_elf, int pltIdx, struct umaincall * callContext)
{
    if (!dasics_strcmp("malloc",_get_lib_name(_elf, pltIdx)) && \
        !(_elf->_flags & MAIN_AREA))
    {
        handle_lib_malloc(callContext);
        callContext->t1 = callContext->ra;
        return 0;
    }

    if (!dasics_strcmp("free",_get_lib_name(_elf, pltIdx)) && \
        !(_elf->_flags & MAIN_AREA))
    {
        handle_lib_free(callContext);
        callContext->t1 = callContext->ra;
        return 0;
    }

    if (!dasics_strcmp("realloc",_get_lib_name(_elf, pltIdx)) && \
        !(_elf->_flags & MAIN_AREA))
    {
        handle_lib_realloc(callContext);
        callContext->t1 = callContext->ra;
        return 0;        
    }        
    
    
    return 1;
}