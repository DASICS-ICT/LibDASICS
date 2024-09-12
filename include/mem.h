#ifndef __INCLUDE_MEM_H
#define __INCLUDE_MEM_H
#include <stdint.h>
#include <stddef.h>

#define DASICS_HOOK_MAGIC 	0xdeadbeef
#define DASICS_MALLOC_HOOK	1
#define DASICS_FREE_HOOK 	2
#define DASICS_REALLOC_HOOK 3
typedef void *(*malloc_hook)(unsigned long, unsigned long ,size_t);
typedef void *(*realloc_hook)(unsigned long, unsigned long , void *, size_t);
typedef void (*free_hook)(unsigned long, unsigned long , void *);

typedef void *(*malloc_handler)(size_t);
typedef void *(*realloc_handler)(void *, size_t);
typedef void (*free_handler)(void *);




extern malloc_handler udasics_malloc_handler;
extern realloc_handler udasics_realloc_handler;
extern free_handler udasics_free_handler;

struct umaincall;


int dasics_openssl_umaincall_hook(struct umaincall * regs);

void * umain_malloc_hook(size_t size);
void * umain_realloc_hook(void * addr, size_t new_size);
void umain_free_hook(void * addr);









#endif