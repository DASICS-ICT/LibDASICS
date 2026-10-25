#include <asm/asm.h>
#include <asm/offset.h>
#include <ucsr.h>
#include <spec.h>

.option push
.option norelax
.section .text

.macro SAVE_MAINCALL
    addi sp, sp, -OFFSET_UMAINCALL
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
    fsd f0, (OFFSET_UMAINCALL_FPR + 0)(sp)
    fsd f1, (OFFSET_UMAINCALL_FPR + 8)(sp)
    fsd f2, (OFFSET_UMAINCALL_FPR + 16)(sp)
    fsd f3, (OFFSET_UMAINCALL_FPR + 24)(sp)
    fsd f4, (OFFSET_UMAINCALL_FPR + 32)(sp)
    fsd f5, (OFFSET_UMAINCALL_FPR + 40)(sp)
    fsd f6, (OFFSET_UMAINCALL_FPR + 48)(sp)
    fsd f7, (OFFSET_UMAINCALL_FPR + 56)(sp)
    fsd f8, (OFFSET_UMAINCALL_FPR + 64)(sp)
    fsd f9, (OFFSET_UMAINCALL_FPR + 72)(sp)
    fsd f10, (OFFSET_UMAINCALL_FPR + 80)(sp)
    fsd f11, (OFFSET_UMAINCALL_FPR + 88)(sp)
    fsd f12, (OFFSET_UMAINCALL_FPR + 96)(sp)
    fsd f13, (OFFSET_UMAINCALL_FPR + 104)(sp)
    fsd f14, (OFFSET_UMAINCALL_FPR + 112)(sp)
    fsd f15, (OFFSET_UMAINCALL_FPR + 120)(sp)
    fsd f16, (OFFSET_UMAINCALL_FPR + 128)(sp)
    fsd f17, (OFFSET_UMAINCALL_FPR + 136)(sp)
    fsd f18, (OFFSET_UMAINCALL_FPR + 144)(sp)
    fsd f19, (OFFSET_UMAINCALL_FPR + 152)(sp)
    fsd f20, (OFFSET_UMAINCALL_FPR + 160)(sp)
    fsd f21, (OFFSET_UMAINCALL_FPR + 168)(sp)
    fsd f22, (OFFSET_UMAINCALL_FPR + 176)(sp)
    fsd f23, (OFFSET_UMAINCALL_FPR + 184)(sp)
    fsd f24, (OFFSET_UMAINCALL_FPR + 192)(sp)
    fsd f25, (OFFSET_UMAINCALL_FPR + 200)(sp)
    fsd f26, (OFFSET_UMAINCALL_FPR + 208)(sp)
    fsd f27, (OFFSET_UMAINCALL_FPR + 216)(sp)
    fsd f28, (OFFSET_UMAINCALL_FPR + 224)(sp)
    fsd f29, (OFFSET_UMAINCALL_FPR + 232)(sp)
    fsd f30, (OFFSET_UMAINCALL_FPR + 240)(sp)
    fsd f31, (OFFSET_UMAINCALL_FPR + 248)(sp)
    frcsr t0
    sd t0, OFFSET_UMAINCALL_FCSR(sp)
    csrr t0, CSR_DRETURNPC
    sd t0, OFFSET_UMAINCALL_RETURNPC(sp)
    csrr t0, CSR_DFZRETURN
    sd t0, OFFSET_UMAINCALL_FZRETURNPC(sp)
.endm

.macro RESTORE_MAINCALL
    ld t0, OFFSET_UMAINCALL_RETURNPC(sp)
    csrw CSR_DRETURNPC, t0
    ld t0, OFFSET_UMAINCALL_FZRETURNPC(sp)
    csrw CSR_DFZRETURN, t0
    ld t0, OFFSET_UMAINCALL_FCSR(sp)
    fscsr t0
    fld f0, (OFFSET_UMAINCALL_FPR + 0)(sp)
    fld f1, (OFFSET_UMAINCALL_FPR + 8)(sp)
    fld f2, (OFFSET_UMAINCALL_FPR + 16)(sp)
    fld f3, (OFFSET_UMAINCALL_FPR + 24)(sp)
    fld f4, (OFFSET_UMAINCALL_FPR + 32)(sp)
    fld f5, (OFFSET_UMAINCALL_FPR + 40)(sp)
    fld f6, (OFFSET_UMAINCALL_FPR + 48)(sp)
    fld f7, (OFFSET_UMAINCALL_FPR + 56)(sp)
    fld f8, (OFFSET_UMAINCALL_FPR + 64)(sp)
    fld f9, (OFFSET_UMAINCALL_FPR + 72)(sp)
    fld f10, (OFFSET_UMAINCALL_FPR + 80)(sp)
    fld f11, (OFFSET_UMAINCALL_FPR + 88)(sp)
    fld f12, (OFFSET_UMAINCALL_FPR + 96)(sp)
    fld f13, (OFFSET_UMAINCALL_FPR + 104)(sp)
    fld f14, (OFFSET_UMAINCALL_FPR + 112)(sp)
    fld f15, (OFFSET_UMAINCALL_FPR + 120)(sp)
    fld f16, (OFFSET_UMAINCALL_FPR + 128)(sp)
    fld f17, (OFFSET_UMAINCALL_FPR + 136)(sp)
    fld f18, (OFFSET_UMAINCALL_FPR + 144)(sp)
    fld f19, (OFFSET_UMAINCALL_FPR + 152)(sp)
    fld f20, (OFFSET_UMAINCALL_FPR + 160)(sp)
    fld f21, (OFFSET_UMAINCALL_FPR + 168)(sp)
    fld f22, (OFFSET_UMAINCALL_FPR + 176)(sp)
    fld f23, (OFFSET_UMAINCALL_FPR + 184)(sp)
    fld f24, (OFFSET_UMAINCALL_FPR + 192)(sp)
    fld f25, (OFFSET_UMAINCALL_FPR + 200)(sp)
    fld f26, (OFFSET_UMAINCALL_FPR + 208)(sp)
    fld f27, (OFFSET_UMAINCALL_FPR + 216)(sp)
    fld f28, (OFFSET_UMAINCALL_FPR + 224)(sp)
    fld f29, (OFFSET_UMAINCALL_FPR + 232)(sp)
    fld f30, (OFFSET_UMAINCALL_FPR + 240)(sp)
    fld f31, (OFFSET_UMAINCALL_FPR + 248)(sp)
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

/* A side frame keeps the target's original stack arguments in place. This
 * process-local stack has the same single-threaded lifetime as dynamic_level;
 * it does not claim protected storage or general nonlocal-unwind support. */
.equ LEGACY_CALL_LIMIT, 4096
.equ LEGACY_CALL_FRAME_SIZE, 128
.macro SAVE_CALL
    la t3, dasics_legacy_call_depth
    ld t4, 0(t3)
    li t5, LEGACY_CALL_LIMIT
    bgeu t4, t5, .Llegacy_fatal
    la t5, dasics_legacy_call_frames
    slli t6, t4, 7
    add t5, t5, t6
    sd ra, 0(t5)
    sd s11, 8(t5)
    csrr t6, CSR_DRETURNPC
    sd t6, 16(t5)
    csrr t6, CSR_DFZRETURN
    sd t6, 24(t5)
    sd sp, 32(t5)
    csrr t6, CSR_DLCFG
    sd t6, 40(t5)
    csrr t6, CSR_DLBOUND0LO
    sd t6, 48(t5)
    csrr t6, CSR_DLBOUND0HI
    sd t6, 56(t5)
    csrr t6, CSR_DJCFG
    sd t6, 64(t5)
    csrr t6, CSR_DJBOUND0LO
    sd t6, 72(t5)
    csrr t6, CSR_DJBOUND0HI
    sd t6, 80(t5)
    la t6, dynamic_level
    ld t6, 0(t6)
    sd t6, 88(t5)
    addi t4, t4, 1
    sd t4, 0(t3)
    mv s11, t5
    .cfi_escape 0x10, 0x01, 0x02, 0x8b, 0x00
    .cfi_escape 0x10, 0x1b, 0x02, 0x8b, 0x08
.endm

.macro RESTORE_CALL restore_grants
    ld t3, 32(s11)
    bne t3, sp, .Llegacy_fatal
    la t3, dasics_legacy_call_depth
    ld t4, 0(t3)
    beqz t4, .Llegacy_fatal
    addi t4, t4, -1
    la t5, dasics_legacy_call_frames
    slli t6, t4, 7
    add t5, t5, t6
    bne t5, s11, .Llegacy_fatal
    sd t4, 0(t3)
    .if \restore_grants
    csrw CSR_DLCFG, zero
    csrw CSR_DJCFG, zero
    ld t3, 48(s11)
    csrw CSR_DLBOUND0LO, t3
    ld t3, 56(s11)
    csrw CSR_DLBOUND0HI, t3
    ld t3, 72(s11)
    csrw CSR_DJBOUND0LO, t3
    ld t3, 80(s11)
    csrw CSR_DJBOUND0HI, t3
    ld t3, 40(s11)
    csrw CSR_DLCFG, t3
    ld t3, 64(s11)
    csrw CSR_DJCFG, t3
    .endif
    ld t3, 16(s11)
    csrw CSR_DRETURNPC, t3
    ld t3, 24(s11)
    csrw CSR_DFZRETURN, t3
    la t3, dynamic_level
    ld t4, 88(s11)
    sd t4, 0(t3)
    ld ra, 0(s11)
    ld s11, 8(s11)
    .cfi_restore ra
    .cfi_restore s11
.endm

ENTRY(dynamic_hook)
    .cfi_startproc
    SAVE_CALL
    la t4, dynamic_level
    ld t5, 0(t4)
    addi t5, t5, 1
    sd t5, 0(t4)
    la t0, _plt_start
    addi t1, t1, -12
    bltu t1, t0, .Llegacy_fatal
    sub t1, t1, t0
    andi t4, t1, 15
    bnez t4, .Llegacy_fatal
    srli t1, t1, 1
    li t4, GOT_NUM * 8
    bgeu t1, t4, .Llegacy_fatal
    la t2, _local_table
    add t1, t2, t1
    ld t2, 0(t1)
    beqz t2, .Llegacy_fatal
    li t5, DASICS_USER_LIMIT
    li t6, DASICS_LIBCFG_V | DASICS_LIBCFG_R | DASICS_LIBCFG_W
    csrw CSR_DLBOUND0LO, zero
    csrw CSR_DLBOUND0HI, t5
    csrw CSR_DLCFG, t6
    li t4, DASICS_LIBRARY_BASE
    li t6, DASICS_JUMPCFG_V
    csrw CSR_DJBOUND0LO, t4
    csrw CSR_DJBOUND0HI, t5
    csrw CSR_DJCFG, t6
    .word DASICS_FDICALL_JR_RA_T2
    RESTORE_CALL 1
    ret
    .cfi_endproc
END(dynamic_hook)

/* The legacy loader invokes this on main's terminal return into libc. Its
 * address-placement protocol remains private to that loader. */
ENTRY(restore_main)
    la t4, dynamic_level
    li t5, 0xffffffffff
    sd t5, 0(t4)
    li t5, DASICS_USER_LIMIT
    li t6, DASICS_LIBCFG_V | DASICS_LIBCFG_R | DASICS_LIBCFG_W
    csrw CSR_DLBOUND0LO, zero
    csrw CSR_DLBOUND0HI, t5
    csrw CSR_DLCFG, t6
    li t4, DASICS_LIBRARY_BASE
    li t6, DASICS_JUMPCFG_V
    csrw CSR_DJBOUND0LO, t4
    csrw CSR_DJBOUND0HI, t5
    csrw CSR_DJCFG, t6
    ret
END(restore_main)

ENTRY(dasics_umaincall)
    SAVE_MAINCALL
    mv a0, sp
    ld t0, umaincall_helper
    beqz t0, .Llegacy_fatal
    jalr t0
    RESTORE_MAINCALL
    jr t1
END(dasics_umaincall)

/* Keep the historical lib_call ABI: a0 is the target address. This entry does
 * not shift variadic registers into another signature or configure grants.
 * Refills performed by the callee keep their CSR bank and handle map intact. */
ENTRY(lib_call)
    .cfi_startproc
    SAVE_CALL
    .word DASICS_FDICALL_JR_RA_A0
    RESTORE_CALL 0
    ret
    .cfi_endproc
END(lib_call)

.Llegacy_fatal:
    li a0, 127
    li a7, 94
    ecall
    j .Llegacy_fatal

.section .bss
.balign 16
dasics_legacy_call_depth:
    .zero 8
.balign 16
dasics_legacy_call_frames:
    .zero LEGACY_CALL_FRAME_SIZE * LEGACY_CALL_LIMIT
.option pop
.section .note.GNU-stack,"",@progbits
