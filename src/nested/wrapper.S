//MAIN FUNCTION
#include <asm/asm.h>

// arg0, arg1, arg2, arg3, dest_addr
ULIBENTRY(dasics_umain_libcall)
    addi   sp, sp, -8
    sd     ra, 0(sp)
    .word 0x0007108b    // dasicscall.jr ra, a4
    ld     ra, 0(sp)
    addi   sp, sp, 8
    ret
END(dasics_umain_libcall)

//ULIB FUNCTION
.section .ulib1text
// arg0, arg1, arg2, arg3, dest_addr
ULIBENTRY(dasics_ulib_libcall)
    addi   sp, sp, -8
    sd     ra, 0(sp)
    .word 0x0007108b    // dasicscall.jr ra, a4
    ld     ra, 0(sp)
    addi   sp, sp, 8
    ret
END(dasics_ulib_libcall)
.section .ulib1text
// bound_src, bound_dst
ULIBENTRY(dasics_ulib_copy_mem_bound)
    .word 0x00b5200b // dibndmv a0, a1, 0
    ret
END(dasics_ulib_copy_mem_bound)
.section .ulib1text
ULIBENTRY(dasics_ulib_copy_jmp_bound)
    .word 0x00b5208b // dibndmv a0, a1, 1
    ret
END(dasics_ulib_copy_jmp_bound)

.section .ulib1text
// void
ULIBENTRY(dasics_ulib_query_mem_bound)
    .word 0x0000350b //dibndquery a0, 0
    ret
END(dasics_ulib_query_mem_bound)

.section .ulib1text
ULIBENTRY(dasics_ulib_query_jmp_bound)
    .word 0x0010350b //dibndquery a0, 1
    ret
END(dasics_ulib_query_jmp_bound)