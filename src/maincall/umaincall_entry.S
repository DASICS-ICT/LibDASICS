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
    addi a0, sp, OFFSET_UMAINCALL
    sd a0, OFFSET_UMAINCALL_SP(sp)
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

.macro SAVE_MAINCALLRET
    addi sp, sp,  -OFFSET_UMAINCALL
    sd a0, OFFSET_UMAINCALL_A0(sp)
.endm


.macro RESTORE_MAINCALLRET
    ld ra, OFFSET_UMAINCALL_RA(sp)
    ld a0, OFFSET_UMAINCALL_A0(sp)
    addi sp, sp, OFFSET_UMAINCALL
.endm


#-----------------------------------------------------------------------
# dasics_umaincall: pass arguments to umaincall helper.
#-----------------------------------------------------------------------
//ENTRY(dasics_umaincall)
//    auipc t4, 0 // t4 = address of dasics_umaincall
//    beq  t3, t4, 1f // if (CallContext->t3 == (reg_t)dasics_umaincall)
//    beq  ra, t4, 2f // if (CallContext->ra == (reg_t)dasics_umaincall) 
//    // umaincall not need to save context, we used temporary registers
//    j 3f    
//1:
//    // t0 is umain_elf_t
//    // t1 is offset of target function
//    addi t1, t1, 16
//
//    // get target function address
//    ld   t2, 0(t0)
//    add  t2, t2, t1
//    ld   t2, 0(t2)
//
//    // is maincall?
//    ld   t5, 32(t0)
//    add  t5, t5, t1
//    ld   t5, 0(t5)
//
//    beqz t5, 5f // not maincall, jump to 5
//
//    li t3, 0 // set t3 to 0 to aviod dasics_umaincall judge fault
//    jr t2 // jump to target function
//    // unreachable
//    ebreak
//
//5:
//    // get target elf
//    ld   t3, 16(t0)
//    add  t3, t3, t1
//    ld   t3, 0(t3)
//
//    // get target function name
//    ld   t4, 24(t0)
//    add  t4, t4, t1
//    ld   t4, 0(t4)
//
//
//    SAVE_MAINCALL 
//    // Let's do dasics_dynamic_call first    
//    // int dasics_dynamic_call(struct umaincall * CallContext, \
//    //                         umain_elf_t * _elf, \
//    //                         int idx, \
//    //                         uint64_t target, \
//    //                         umain_elf_t * target_elf, \
//    //                         const char * target_name)    
//    
//    mv   a0, sp
//    mv   a1, t0
//    srli a2, t1, 3
//    mv   a3, t2
//    mv   a4, t3
//    mv   a5, t4
//    call dasics_dynamic_call
//    j 6f
//2:
//    SAVE_MAINCALLRET
//    mv   a0, sp
//    // ra is the address of dasics_umaincall
//    call dasics_dynamic_return
//    j 4f
//3:
//    ld   t4, umaincall_helper
//    jalr t4
//    ret
//4:
//    RESTORE_MAINCALLRET
//    ret
//6:
//    RESTORE_MAINCALL
//    jr t1
//    ebreak
//END(dasics_umaincall)
ENTRY(dasics_umaincall)
    la t1, __umain_call_table
    slli t0, t0, 3
    add t0, t0, t1
    ld t0, 0(t0)
    jr t0
    ret
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
