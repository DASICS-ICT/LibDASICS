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


#    addi sp, sp,  OFFSET_UMAINCALL
.endm




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
