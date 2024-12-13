#ifndef _INCLUDE_DYNAMIC_H
#define _INCLUDE_DYNAMIC_H

#include <stdint.h>
#include <stddef.h>
#include <link.h>
#include <elf.h>
#include <list.h>
#include <dasics_stdio.h>
#include <dasics_string.h>

#define PAGE_SIZE 0x1000

typedef uint64_t (*fixup_entry_t)(uint64_t, uint64_t);
struct func_mem;


#define RANGE(X, Y, Z) (((X)>(Y)) && ((X)<(Z)))
#define D_PTR(map,i) map->i->d_un.d_ptr
#define PLTREL  ElfW(Rela)

#define ELFW(type)	_ElfW (ELF, __ELF_NATIVE_CLASS, type)

/* Define the find result of the plt */
#define NOEXIST -1


typedef struct umain_elf
{
   uint64_t *_local_got_table; /* Num of lib call */   
   uint64_t *_local_call_time; /* Call num of a outer func */ 
   uint64_t calculate;

   ElfW(Addr) l_addr;		/* Difference between the address in the ELF
            file and the addresses in memory.  */
   char l_name[256];	      /* Module name, for 256 */
   
   /* real_name */
   char * real_name;

   /* For the list */
   struct umain_elf * umain_elf_next, *umain_elf_prev;

   // fixup handler and arg
   fixup_entry_t fixup_handler;
   struct link_map *map_link;
   /* 
    * If the lib is a trusted, the _point_got will be the pointed untrusted lib
    * else it will be a point trusted lib (if the lib has a pointed)
    * 
    * Or be NULL
    */
   struct umain_elf * _copy_lib_elf;

   list_head func_man;  /* manage func's memory */

   /* The useful message of the module */
   ElfW(Dyn) *l_info[DT_NUM];
   /* The plt begin which will be used to count the got, it can be read from got[2], got[3]... */
   uint64_t *got_begin  ;
   uint64_t *plt_begin  ;
   uint64_t got_num     ;
   uint64_t dynamic     ;		/* Dynamic section of the shared object.  */

   struct func_mem **local_func; /* Find func_mem fast */
   int * redirect_switch;       /* Redirect switch */
   struct umain_elf ** target_elf; /* Target elf */
   char **target_func_name;

   struct func_mem * namespace_func;

   const ElfW(Phdr) *l_phdr;	/* Pointer to program header table in core.  */
   const ElfW(Ehdr) *l_ehdr;  /* Pointer to elf header table in core.  */
   ElfW(Addr) l_entry;		   /* Entry point location.  */
   ElfW(Half) l_phnum;		   /* Number of program header entries.  */
   

   uint64_t _flags;           /* Define in dasics_link_manager */

   ElfW(Addr) l_relro_addr;
   ElfW(Addr) l_relro_size; 

   /* record the _text, plt, r_only_area, _w_area(imply read) */
   uint64_t _text_start, _text_end;
   uint64_t _plt_start, _plt_end;
   uint64_t _r_start, _r_end;
   uint64_t _w_start, _w_end; 
   uint64_t _map_start, _map_end;
} umain_elf_t;


extern fixup_entry_t dll_fixup_handler;
extern fixup_entry_t dll_fixup_handler_lib;
extern uint64_t user_sp;
extern umain_elf_t * _umain_elf_table;
extern uint64_t tp_stage1_ld;

// Creat Umain elf 
int create_umain_elf_chain(struct link_map * main_elf);
void open_memory(umain_elf_t * _main);
struct link_map * get_main_link();
void init_elf_plt(umain_elf_t * elf, uint32_t * pltPc, uint64_t * gotAddr);

// check is elf
static inline int is_elf_format(char *binary)
{

   const ElfW(Ehdr) *ehdr = (ElfW(Ehdr) *)binary;
    if (ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
        ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
        ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
        ehdr->e_ident[EI_MAG3] == ELFMAG3) {
        return 1;
    }

    return 0;
}

// get the area of the pc
static inline umain_elf_t * _get_area(uint64_t pc)
{
   umain_elf_t * _elf = _umain_elf_table;

   if (_umain_elf_table == NULL) return NULL; 

   do {
        if (RANGE(pc, \
                _elf ->_text_start, \
                _elf ->_text_end)
            )
            return _elf;
        _elf = _elf->umain_elf_next;

    } while (_elf != _umain_elf_table);
    
    
    return NULL;
}

static inline umain_elf_t * _get_area_by_name(const char * name)
{
   umain_elf_t * _elf = _umain_elf_table;

   if (_umain_elf_table == NULL) return NULL; 

   do {
        if (!dasics_strcmp(_elf->real_name, name))
            return _elf;
        _elf = _elf->umain_elf_next;

    } while (_elf != _umain_elf_table);
    
    
    return NULL;    
}

/*
 * This function is used to get the library's func name by the index
 */
static inline char * _get_lib_name(umain_elf_t * entry, uint64_t plt_idx)
{

    ElfW(Word) reloc_arg = (((uint64_t)plt_idx * 0x10UL) >> 1) * 3;
    ElfW(Sym) *const symtab
        = (void *) (D_PTR (entry, l_info[DT_SYMTAB]) + entry->l_addr);
    char *strtab = (void *) (D_PTR (entry, l_info[DT_STRTAB]) + entry->l_addr);

    uintptr_t pltgot = (uintptr_t) (D_PTR (entry, l_info[DT_PLTGOT]));   

    PLTREL *const reloc
        = (void *) ((D_PTR (entry, l_info[DT_JMPREL]) + entry->l_addr)
                + reloc_arg);    
    ElfW(Sym) *sym = &symtab[ELFW(R_SYM) (reloc->r_info)];

    return strtab + sym->st_name;
}



/*
 * This function is used to figure out which plt[x] the uepc was seted,
 * if find, return the idx or retuen -1;
 *
 */
static inline int _is_plt_area(uint64_t uepc, umain_elf_t * _got_entry)
{
    if (RANGE(uepc, \
             _got_entry->_plt_start, \
             _got_entry->_plt_end))
    {
        return (uepc - _got_entry->_plt_start) / 0x10UL;
    }

    return NOEXIST;
}


#endif