#include <asm/asm.h>

// inline BRK
ENTRY(__BRK)
    li a7, 214
    ecall
    ret
END(__BRK)