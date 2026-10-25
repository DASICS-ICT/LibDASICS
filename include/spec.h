#ifndef __INCLUDE_SPEC_H
#define __INCLUDE_SPEC_H

#define GOT_NUM 4096

#ifndef __ASSEMBLER__
#include <stdint.h>

// Local function table
extern uint64_t _local_table[GOT_NUM];



void ignore_simple_function();


#endif
#endif