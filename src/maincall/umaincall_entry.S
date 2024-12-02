#include <asm/asm.h>
#include <asm/offset.h>
#include <ucsr.h>

// MAINCALL used registers
.macro SAVE_MAINCALL
    addi sp, sp,  -OFFSET_UMAINCALL

    sd t0, OFFSET_UMAINCALL_T0(sp)
    sd t1, OFFSET_UMAINCALL_T1(sp)
    sd t3, OFFSET_UMAINCALL_T3(sp)
    sd ra, OFFSET_UMAINCALL_RA(sp)
    sd a0, OFFSET_UMAINCALL_A0(sp)
    sd a1, OFFSET_UMAINCALL_A1(sp)
    sd a2, OFFSET_UMAINCALL_A2(sp)
    sd a3, OFFSET_UMAINCALL_A3(sp)
    sd a4, OFFSET_UMAINCALL_A4(sp)
    sd a5, OFFSET_UMAINCALL_A5(sp)
    sd a6, OFFSET_UMAINCALL_A6(sp)
    sd a7, OFFSET_UMAINCALL_A7(sp)
    addi t0, sp, OFFSET_UMAINCALL
    sd t0, OFFSET_UMAINCALL_SP(sp)
.endm


.macro RESTORE_MAINCALL
    ld t0, OFFSET_UMAINCALL_T0(sp)
    ld t1, OFFSET_UMAINCALL_T1(sp)
    ld t3, OFFSET_UMAINCALL_T3(sp)
    ld ra, OFFSET_UMAINCALL_RA(sp)
    ld a0, OFFSET_UMAINCALL_A0(sp)
    ld a1, OFFSET_UMAINCALL_A1(sp)
    ld a2, OFFSET_UMAINCALL_A2(sp)
    ld a3, OFFSET_UMAINCALL_A3(sp)
    ld a4, OFFSET_UMAINCALL_A4(sp)
    ld a5, OFFSET_UMAINCALL_A5(sp)
    ld a6, OFFSET_UMAINCALL_A6(sp)
    ld a7, OFFSET_UMAINCALL_A7(sp)
    ld sp, OFFSET_UMAINCALL_SP(sp)
.endm

#-----------------------------------------------------------------------
# dynamic hook: dynamic call, set hook for outer function call
#-----------------------------------------------------------------------
# t0 will be umain_elf_t
# t1 will be the offset
# never use parameter transfer register
ENTRY(dynamic_hook)
    addi sp, sp, -8
    sd ra, 0(sp)

    la t4, dynamic_level
    ld t5, 0(t4)
    addi t5, t5, 1
    sd t5, 0(t4)

    ld t2, (t0)
    add t2, t2, t1
    ld t2, 16(t2)

// #ifdef DASICS_LINUX
//     csrw CSR_DRETURNPC, ra
// #endif
//     fence.i
//     jr t2
    li t4, 0
    li t5, 0x4000000000
    li t6, 0xb

    csrw CSR_DLBOUND0LO, t4
    csrw CSR_DLBOUND0HI, t5
    csrw CSR_DLCFG, t6

    li t4, 0x2000000000
    li t6, 0x1
    csrw CSR_DJBOUND0LO, t4
    csrw CSR_DJBOUND0HI, t5    
    csrw CSR_DJCFG, t6

    .word 0x0003908b    // dasicscall.jr ra, t2
//     csrw CSR_DRETURNPC, ra
//     fence.i
//     jr t2

    la t4, dynamic_level
    ld t5, 0(t4)
    addi t5, t5, -1
    sd t5, 0(t4)

    bnez t5, 1f


    csrw CSR_DLBOUND1LO, zero
    csrw CSR_DLBOUND1HI, zero
    csrw CSR_DLCFG, zero

    csrw CSR_DJBOUND0LO, zero
    csrw CSR_DJBOUND0HI, zero
    csrw CSR_DJCFG, zero

1:
    // la t3, dynamic_hook_ra
    // ld ra, 0(t3)
    ld ra, 0(sp)
    addi sp, sp, 8
    ret
END(dynamic_hook)

#-----------------------------------------------------------------------
# restore main: Function main return to __libc_start_main which located
# in untrusted area, so set privilige
#-----------------------------------------------------------------------
# never use parameter transfer register
ENTRY(restore_main)
    la t4, dynamic_level
    li t5, 0xffffffffff
    sd t5, 0(t4)

    li t4, 0
    li t5, 0x4000000000
    li t6, 0xb

    csrw CSR_DLBOUND0LO, t4
    csrw CSR_DLBOUND0HI, t5
    csrw CSR_DLCFG, t6

    li t4, 0x2000000000
    li t6, 0x1
    csrw CSR_DJBOUND0LO, t4
    csrw CSR_DJBOUND0HI, t5    
    csrw CSR_DJCFG, t6
    ret
END(restore_main)


#-----------------------------------------------------------------------
# dasics_umaincall: pass arguments to umaincall helper.
#-----------------------------------------------------------------------
ENTRY(dasics_umaincall)
    SAVE_MAINCALL 
    mv   a0, sp
    ld   t0, umaincall_helper
    jalr t0
    RESTORE_MAINCALL
    jr t1
END(dasics_umaincall)


#-----------------------------------------------------------------------
# : pass arguments to umaincall helper.
#-----------------------------------------------------------------------
.section .text
.global lib_call
.type lib_call, @function

lib_call:
    addi   sp, sp, -8
    sd     ra, 0(sp)
    .word 0x0005108b    // dasicscall.jr ra, a0
    ld     ra, 0(sp)
    addi   sp, sp, 8
    ret
