#ifndef __INCLUDE_HOOK_H
#define __INCLUDE_HOOK_H

#include <stdint.h>

#define DASICS_HOOK_FUNC_MAGIC 0x66668888

struct umaincall;

uint64_t dasics_func_pointer_hook(struct umaincall * regs);


#endif