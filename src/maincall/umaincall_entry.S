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
    addi   sp, sp, -STACK_FRAME_SIZE_LIBCALL
    sd     ra, 64(sp)
    sd     s0, 56(sp)
    addi   s0, sp, STACK_FRAME_SIZE_LIBCALL

    // Save registers
    sd     a1, -24(s0)
    sd     a2, -32(s0)
    sd     a3, -40(s0)
    sd     a4, -48(s0)
    sd     a5, -56(s0)
    sd     a6, -64(s0)
    sd     a7, -72(s0)

    // Change the order of arguments
    mv     t0, a0
    ld     a0, -24(s0)
    ld     a1, -32(s0)
    ld     a2, -40(s0)
    ld     a3, -48(s0)
    ld     a4, -56(s0)
    ld     a5, -64(s0)
    ld     a6, -72(s0)

    // Call the target function
    .word 0x0002908b    // dasicscall.jr ra, t0

    // Restore registers
    ld     ra, 64(sp)
    ld     s0, 56(sp)
    addi   sp, sp, STACK_FRAME_SIZE_LIBCALL
    ret
