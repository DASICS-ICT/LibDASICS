#ifndef __INCLUDE_SPEC_H
#define __INCLUDE_SPEC_H

#include <stdint.h>
#define GOT_NUM 4096

// Local function table
extern uint64_t _local_table[GOT_NUM];



void ignore_simple_function();


#endif