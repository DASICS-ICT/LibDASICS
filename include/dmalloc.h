#ifndef _INCLUDE_DMALLOC_H
#define _INCLUDE_DMALLOC_H

#include <stdint.h>
#include <sys/mman.h>

// Chunk size be 16KB
#define CHUNK_SIZE 0x40000UL

extern void * _dasics_mmap(void * addr, uint64_t length, int prot, int flags, int fd, uint64_t offset);
extern int _dasics_mprotect(void * addr, uint64_t length, int prot);
void * dasics_malloc(uint64_t size);





#endif