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

#-----------------------------------------------------------------------
# dasics_umaincall: pass arguments to umaincall helper.
#-----------------------------------------------------------------------
ENTRY(dasics_umaincall)
    auipc t4, 0 // t4 = address of dasics_umaincall
    beq  t3, t4, 1f // if (CallContext->t3 == (reg_t)dasics_umaincall)
    // umaincall not need to save context, we used temporary registers
    j 2f
1:
    // t0 = source umain_elf_t, t1 = raw PLT offset
    addi t1, t1, 16

    // Resolve target function address from _local_got_table
    ld   t2, 0(t0)
    add  t2, t2, t1
    ld   t2, 0(t2)

    // Check if target is in MAIN_AREA (trusted direct jump, no transition)
    ld   t5, 32(t0)
    add  t5, t5, t1
    ld   t5, 0(t5)

    beqz t5, 5f // not maincall, jump to 5

    li t3, 0 // set t3 to 0 to aviod dasics_umaincall judge fault
    jr t2 // jump to target function
    // unreachable
    ebreak

5:
    // Resolve target_elf pointer for do_transition_dynamic
    ld   t3, 16(t0)
    add  t3, t3, t1
    ld   t3, 0(t3)

    /*
     * do_transition_dynamic(func, saved_regs, target_elf)
     *   a0 = target function address
     *   a1 = pointer to saved {a0..a7} in the umaincall stack frame
     *   a2 = target_elf (passed explicitly to skip runtime lookup)
     */
    mv   a0, t2
    addi a1, sp, OFFSET_UMAINCALL_A0
    mv   a2, t3
    call do_transition_dynamic

    // Overwrite saved a0 with the callee's return value
    sd   a0, OFFSET_UMAINCALL_A0(sp)

    RESTORE_MAINCALL
    ret
2:
    ld   t4, umaincall_helper
    jr   t4
    ret
END(dasics_umaincall)
