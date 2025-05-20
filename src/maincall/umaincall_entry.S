#include <asm/asm.h>
#include <asm/offset.h>

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
# dasics_umaincall: pass arguments to umaincall helper.
#-----------------------------------------------------------------------
ENTRY(dasics_umaincall)
    SAVE_MAINCALL 
    mv   a0, sp
    // Let's do dasics_dynamic_call first
    call dasics_dynamic_call
    bnez a0, 1f
    mv   a0, sp
    ld   t4, umaincall_helper
    jalr t4
1:
    RESTORE_MAINCALL
    jr t1
END(dasics_umaincall)


#-----------------------------------------------------------------------
# lib_call: pass arguments to the target function.
#-----------------------------------------------------------------------
.section .text
.global lib_call
.type lib_call, @function

lib_call:
    // Allocate stack frame
    addi   sp, sp, -72
    sd     ra, 64(sp)
    sd     s0, 56(sp)
    addi   s0, sp, 72

    // Save registers
    sd     a1, -16(sp)
    sd     a2, -24(sp)
    sd     a3, -32(sp)
    sd     a4, -40(sp)
    sd     a5, -48(sp)
    sd     a6, -56(sp)
    sd     a7, -64(sp)

    // Change the order of arguments
    mv     t0, a0
    ld     a0, -16(sp)
    ld     a1, -24(sp)
    ld     a2, -32(sp)
    ld     a3, -40(sp)
    ld     a4, -48(sp)
    ld     a5, -56(sp)
    ld     a6, -64(sp)

    // Call the target function
    .word 0x0002908b    // dasicscall.jr ra, t0

    // Restore registers
    ld     ra, 64(sp)
    ld     s0, 56(sp)
    addi   sp, sp, 72
    ret
