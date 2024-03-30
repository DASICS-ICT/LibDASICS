#ifndef _INCLUDE_UDIRECT_H
#define _INCLUDE_UDIRECT_H
#include <stddef.h>
#include <stdint.h>
#include <list.h>


typedef struct umain_elf umain_elf_t;

uint64_t _call_reloc(umain_elf_t *elf, uint64_t target);


/* Designed to add a func which redirect to untrusted area */
typedef struct redirect
{
    char name[256];

    list_head list;
} redirect_t;

// The list
extern list_head redirect_table;
// The switch
extern int redirect_switch;


/* Add one item  */
int add_redirect_item(const char *func_name);


/* Delete one item from list */ 
int delete_redirect_item(const char *func_name);

typedef struct umain_elf umain_elf_t;
/* Force relocation */
uint64_t force_redirect(umain_elf_t * entry, int idx, uint64_t target);



// Open/Close the switch
int open_redirect();
int close_redirect();











#endif