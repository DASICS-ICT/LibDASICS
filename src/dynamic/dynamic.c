#include <elf.h>
#include <link.h>

#include <dynamic.h>
#include <dasics_string.h>
#include <stddef.h>

umain_elf_t * _umain_elf_table = NULL;


extern uint64_t _interp_start[];
/*
 * This fuinction is used to init umain dasics_link_map
 * include the exe, linker, all library
 */
int create_umain_got_chain(struct link_map * __map, char * name)
{
    /* align the heap for 128bit */
    _umain_elf_table = (umain_elf_t *)__BRK((uint64_t)ROUND(__BRK(NULL), 0x16UL));


    


}






