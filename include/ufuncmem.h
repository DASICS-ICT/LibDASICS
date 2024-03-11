#ifndef _INCLUDE_UFUNCMEM_H
#define _INCLUDE_UFUNCMEM_H
#include <stdint.h>
#include <list.h>

#define MAX_BOUNS 16


// The malloc area; if addr == 0, invalid
struct bound_table {
    uint64_t addr;
    uint64_t length;  
    int handler;
    int index;
};

// Func is func addr
struct func_mem {
    list_head list;
    uint64_t func;
    uint64_t bound_num;
    uint64_t bound_max;
    struct bound_table *mem;
};

#define EXPAN_BOUNDS (sizeof(struct bound_table) * MAX_BOUNS)

extern struct func_mem * global_func_mem;

typedef struct umain_elf umain_elf_t;
struct umaincall;

// Used to handle all malloc, free, realloc of lib func
int handle_lib_mem(umain_elf_t *_elf, int pltIdx, struct umaincall * callContext);

// Handle memory option
void * handle_lib_malloc(struct umaincall * callContext);
int handle_lib_realloc(struct umaincall * callContext);
int handle_lib_free(struct umaincall * callContext);
int set_global_func_man(umain_elf_t *entry, uint64_t func);



#endif