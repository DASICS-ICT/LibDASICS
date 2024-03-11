#ifndef _INCLUDE_DASICS_START_H
#define _INCLUDE_DASICS_START_H

#include <elf.h>
#include <dasics_string.h>

// DASICS INIT STAGE
typedef void (*rtld_fini) (void);
void _dasics_entry_stage1(uint64_t sp, rtld_fini fini);
void _dasics_entry_stage2(uint64_t sp, rtld_fini fini);
void _dasics_entry_stage3(uint64_t sp, rtld_fini fini);

extern void _check_dasics();
extern void _setup_mainlib_entry();
extern void _setup_copy_lib_entry();
extern void _setup_fault();

// DASICS INIT STAGE FALUT AND LIB BOUNDS
int32_t original_libcfg_alloc(uint64_t cfg, uint64_t lo, uint64_t hi);
int32_t original_libcfg_free(int32_t idx);
int32_t original_libcfg_free_all();
int32_t original_libcfg_get(int32_t idx);
int32_t original_jumpcfg_alloc(uint64_t lo, uint64_t hi);
int32_t original_jumpcfg_free(int32_t idx);
int32_t original_jumpcfg_free_all();

// FAULT
struct ucontext_trap;
void dasics_start_fault(struct ucontext_trap * regs);


// DASICS linker auxv define
#define AT_LINKER 55
#define AT_FIXUP 56
#define AT_DASICS 57
#define AT_LINKER_COPY 58
#define AT_TRUST_BASE 59

// Goto linker
#define RESET_ENTRY(sp, entry)({       \
            asm volatile ("mv sp , %0" : "+r"(sp)); \
            asm volatile ("mv ra , %0" : "+r"(entry));          \
            asm volatile ("ret");          \
        })

#define TASK_SIZE 0X4000000000


// DASICS module flag
// Define the area flags
#define MAIN_AREA 0x1UL         /* The main function */
#define LIB_AREA 0x2UL          /* The lib function */
#define LINK_AREA 0x4UL         /* The link function, accompay with MAIN_AREA */
#define ELF_AREA 0x8UL          /* The ELF function, accompay with MAIN_AREA */


extern uint64_t user_sp;
extern int dasics_stage;


/* Get the auxv array addr */ 
static inline uint64_t _get_auxv(uint64_t *sp)
{
    /* skip argc */
    sp ++;

    /* skip argv[] */
    while (*sp)
        sp ++;

    sp ++;

    /* skip envc[] */
    while (*sp)
        sp ++;

    sp ++;

    return (uint64_t)sp;
}


/* get elf entry from the auxv */
static inline uint64_t _get_auxv_entry(uint64_t sp, uint64_t at_id)
{
    Elf64_auxv_t * auxv = (Elf64_auxv_t *)_get_auxv((uint64_t *)sp);
    uint64_t elf_entry = 0;

    while (auxv->a_type)
    {
        if (auxv->a_type == at_id)
        {
            elf_entry = auxv->a_un.a_val;
            break;
        }         
        auxv ++;
    }
    return elf_entry;

}

/* set elf entry from the auxv */
static inline void _set_auxv_entry(uint64_t sp, uint64_t at_id, uint64_t num)
{
    Elf64_auxv_t * auxv = (Elf64_auxv_t *)_get_auxv((uint64_t *)sp);

    while (auxv->a_type)
    {
        if (auxv->a_type == at_id)
        {
            auxv->a_un.a_val = num;
            break;
        }        
        auxv ++;
    }
    
}



#endif