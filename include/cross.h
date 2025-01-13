#ifndef _INCLUDE_CROSS_H
#define _INCLUDE_CROSS_H


#include <stddef.h>
#include <stdint.h>
#include <udasics.h>

#define MAX_DEPTH 4096

// import umian_elf_t from dynamic.h
typedef struct umain_elf umain_elf_t;
struct umaincall; 
struct func_mem;

typedef uint64_t reg_t;

extern uint64_t cross_stack;

struct cross {
    umain_elf_t * begin;
    umain_elf_t * target;
    struct func_mem * func;
    // Return Address
    reg_t ra;
    int handle_num;
    int32_t jmpcfg[DASICS_JUMPCFG_WIDTH];
    // Jumpcall's handler
    int jmp_num;
    int handle[DASICS_LIBCFG_WIDTH];
};

// Init cross stack
void init_cross_stack();

/* Push and Pop */ 
void push_cross(struct cross * tmp);
void pop_cross(struct umaincall * maincallContext);



#endif