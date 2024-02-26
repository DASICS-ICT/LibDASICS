#ifndef _INCLUDE_DYNAMIC_H
#define _INCLUDE_DYNAMIC_H

#include <link.h>
#include <stdint.h>

#define LIB_NUM 256

/* Rounding; only works for n = power of two */
#define ROUND(a, n)     (((((uint64_t)(a))+(n)-1)) & ~((n)-1))

#define ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))

typedef uint64_t (*fixup_entry_t)(uint64_t, uint64_t);

typedef struct umain_elf
{
   ElfW(Addr) l_addr;		/* Difference between the address in the ELF
            file and the addresses in memory.  */
   char l_name[256];	      /* Module name, for 256 */
   
   /* real_name */
   char * real_name;

   /* For the list */
   struct umain_got * umain_got_next, *umain_got_prev;

   // fixup handler and arg
   fixup_entry_t fixup_handler;
   struct link_map *map_link;
   /* 
    * If the lib is a trusted, the _point_got will be the pointed untrusted lib
    * else it will be a point trusted lib (if the lib has a pointed)
    * 
    * Or be NULL
    */
   struct umain_got * _point_got;

   /* The useful message of the module */
   ElfW(Dyn) *l_info[DT_NUM + DT_VERSIONTAGNUM
         + DT_EXTRANUM + DT_VALNUM + DT_ADDRNUM];
   /* The plt begin which will be used to count the got, it can be read from got[2], got[3]... */
   uint64_t *got_begin  ;
   uint64_t *plt_begin  ;
   uint64_t got_num     ;
   uint64_t dynamic     ;		/* Dynamic section of the shared object.  */

   uint64_t _local_got[LIB_NUM]; /* Num of lib call */

   const ElfW(Phdr) *l_phdr;	/* Pointer to program header table in core.  */
   ElfW(Addr) l_entry;		   /* Entry point location.  */
   ElfW(Half) l_phnum;		   /* Number of program header entries.  */
   ElfW(Half) l_ldnum;	   	/* Number of dynamic segment entries.  */
   

   uint64_t _flags;           /* Define in dasics_link_manager */


   /* record the _text, plt, r_only_area, _w_area(imply read) */
   uint64_t _text_start, _text_end;
   uint64_t _plt_start, _plt_end;
   uint64_t _r_start, _r_end;
   uint64_t _w_start, _w_end; 
   uint64_t _map_start, _map_end;
} umain_elf_t;





#endif