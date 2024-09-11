#include <ufuncmem.h>
#include <utrap.h>
#include <umaincall.h>
#include <udasics.h>
#include <dynamic.h>
#include <mem.h>
#include <dasics_start.h>
#include <dasics_string.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

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
    dasics_memset((void *)(&tmp->mem[tmp->bound_max]), 0, EXPAN_BOUNDS);
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

    entry->namespace_func = result;
    return 0;

}


// Handle mem
void * handle_lib_malloc(struct umaincall * callContext, struct func_mem * mem)
{
    // Expand
    if (mem->bound_num == mem->bound_max)
        _expan_bound(mem);

    void * malloc_mem = malloc(callContext->a0);
    assert(malloc_mem != NULL);

    struct bound_table * record = NULL;
    // Find a place to set
    for (int i = 0; i < mem->bound_max; i++)
    {

        if (mem->mem[i].addr == 0)
        {
            mem->mem[i].addr = (uint64_t)malloc_mem;
            mem->mem[i].length = callContext->a0;
            record = &mem->mem[i];
            break;
        }
    }
    mem->bound_num++;
    // Return result to caller
    callContext->a0 = (uint64_t)malloc_mem;


    int32_t handler = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, record->addr, record->length);
    record->handler = handler;

    return 0;
}

// Handle realloc
int handle_lib_realloc(struct umaincall * callContext, struct func_mem * mem)
{

    struct bound_table * bounds = mem->mem;

    int realloc_idx = -1;

    struct bound_table * target = NULL;
    for (int i = 0; i < mem->bound_max; i++)
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

    target->handler = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, target->addr, target->length);
    // Return result to caller
    callContext->a0 = (uint64_t)new_addr;

    

    return 0;
}


// Handle free
int handle_lib_free(struct umaincall * callContext, struct func_mem * mem)
{

    struct bound_table * bounds = mem->mem;


    struct bound_table * target = NULL;
    for (int i = 0; i < mem->bound_max; i++)
    {
        if (bounds[i].addr == callContext->a0)
        {
            target = &bounds[i];
            break;
        }
    }

    assert(target != NULL);    

    free((void *)callContext->a0);

    mem->bound_num--;

    target->addr = 0;
    target->length = 0;

    // Return result to caller
    callContext->a0 = 0;    
    
    dasics_libcfg_free(target->handler);

    target->handler = -1;

    return 0;
}


int handle_lib_mem(umain_elf_t *_elf, const char * name, struct umaincall * callContext)
{
    if (!dasics_strcmp("malloc", name) && \
        !(_elf->_flags & MAIN_AREA))
    {
        handle_lib_malloc(callContext, _elf->namespace_func);
        callContext->t1 = callContext->ra;
        return 0;
    }

    if (!dasics_strcmp("free", name) && \
        !(_elf->_flags & MAIN_AREA))
    {
        handle_lib_free(callContext, _elf->namespace_func);
        callContext->t1 = callContext->ra;
        return 0;
    }

    if (!dasics_strcmp("realloc", name) && \
        !(_elf->_flags & MAIN_AREA))
    {
        handle_lib_realloc(callContext, _elf->namespace_func);
        callContext->t1 = callContext->ra;
        return 0;        
    }        
    
    
    return 1;
}

int init_mem_handler()
{
    void *handle = dlopen("libc.so.6", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error opening libc: %s\n", dlerror());
        return 1;
    }

    udasics_malloc_handler = dlsym(handle, "malloc");
    if (!udasics_malloc_handler) {
        fprintf(stderr, "Error finding malloc: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

    udasics_free_handler = dlsym(handle, "free");
    if (!udasics_free_handler) {
        fprintf(stderr, "Error finding free: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

    udasics_realloc_handler = dlsym(handle, "realloc");
    if (!udasics_realloc_handler) {
        fprintf(stderr, "Error finding realloc: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

    dlclose(handle);

    return 0;    
}