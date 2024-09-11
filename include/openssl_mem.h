#ifndef __INCLUDE_OPENSSL_MEM_H
#define __INCLUDE_OPENSSL_MEM_H
#include <stdint.h>

#define MB 0x100000UL

extern void * openssl_self_heap;

struct umaincall;
void init_openssl_self_heap(uint64_t size);

int dasics_openssl_umaincall_hook(struct umaincall * regs);









#endif