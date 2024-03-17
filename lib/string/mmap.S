#include <asm/asm.h>

ENTRY(_dasics_mmap)
    li a7, 222
    ecall
    ret
END(_dasics_mmap)
