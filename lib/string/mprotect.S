#include <asm/asm.h>

ENTRY(_dasics_mprotect)
    li a7, 226
    ecall
    ret
END(_dasics_mprotect)