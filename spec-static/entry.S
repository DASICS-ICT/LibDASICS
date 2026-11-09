#include "runtime.h"

    .option push
    .option norelax
    .section .text
    .balign 8
    .global _fdi_spec_start
    .type _fdi_spec_start, @function
_fdi_spec_start:
    la gp, __global_pointer$
    addi sp, sp, -16
    sd a0, 0(sp)
    call fdi_spec_initialize
    addi a0, sp, 16
    call fdi_spec_loader_initialize
    ld a0, 0(sp)
    addi sp, sp, 16
    tail _start
    .size _fdi_spec_start, .-_fdi_spec_start

    .balign 8
    .global umain_main
    .type umain_main, @function
umain_main:
    .cfi_startproc
    addi sp, sp, -16
    .cfi_def_cfa_offset 16
    sd ra, 8(sp)
    .cfi_offset ra, -8
    call main
    sd a0, 0(sp)
    call fdi_spec_main_return
    ld a0, 0(sp)
    ld ra, 8(sp)
    .cfi_restore ra
    addi sp, sp, 16
    .cfi_def_cfa_offset 0
    ret
    .cfi_endproc
    .size umain_main, .-umain_main

    .balign 8
    .global fdi_spec_invoke_callback
    .type fdi_spec_invoke_callback, @function
fdi_spec_invoke_callback:
    mv t0, a0
    mv a0, a1
    la t1, fdi_spec_callback_id
    ld t1, 0(t1)
    /* The tail pseudo-instruction would overwrite the live ID in t1. */
    lla t2, fdi_spec_call
    jr t2
    .size fdi_spec_invoke_callback, .-fdi_spec_invoke_callback

    .balign 8
    .global fdi_spec_maincall
    .type fdi_spec_maincall, @function
fdi_spec_maincall:
    .cfi_startproc
    /* Only the enumerated fixed services enter through DMAINCALL. Their
     * arguments fit in a0..a2, so an ordinary aligned ABI frame is sufficient.
     * The gate never turns a jump fault into a permitted arbitrary callback. */
    addi sp, sp, -32
    .cfi_def_cfa_offset 32
    sd ra, 24(sp)
    .cfi_offset ra, -8
    csrr t1, CSR_DRETURNPC
    sd t1, 16(sp)
    csrr t1, CSR_DFZRETURN
    sd t1, 8(sp)
    mv a3, t0
    call fdi_spec_service
    ld t1, 16(sp)
    csrw CSR_DRETURNPC, t1
    ld t1, 8(sp)
    csrw CSR_DFZRETURN, t1
    ld ra, 24(sp)
    .cfi_restore ra
    addi sp, sp, 32
    .cfi_def_cfa_offset 0
    ret
    .cfi_endproc
    .size fdi_spec_maincall, .-fdi_spec_maincall

    .macro TRUSTED_SERVICE name, target
    .balign 8
    .global \name
    .type \name, @function
\name:
    tail \target
    .size \name, .-\name
    .endm
    TRUSTED_SERVICE __wrap_umain___cxa_atexit, fdi_spec_cxa_atexit
    TRUSTED_SERVICE __wrap_umain___cxa_finalize, fdi_spec_cxa_finalize
    TRUSTED_SERVICE __wrap_umain_exit, fdi_spec_process_exit
    TRUSTED_SERVICE __wrap_umain__exit, fdi_spec_direct_exit

    .balign 8
    .global fdi_spec_call
    .type fdi_spec_call, @function
fdi_spec_call:
    .cfi_startproc
    /* t0 is the target and t1 is the generated wrapper ID. Keep sp unchanged:
     * the target must see all stack arguments at their original addresses. */
    la t2, __fdi_untrusted_start
    bltu t0, t2, .Lbad_call
    la t2, __fdi_untrusted_end
    bgeu t0, t2, .Lbad_call
    la t2, fdi_spec_wrapper_count
    ld t2, 0(t2)
    bgeu t1, t2, .Lbad_call
    la t2, fdi_spec_call_depth
    ld t3, 0(t2)
    li t4, FDI_SPEC_FRAME_LIMIT
    bgeu t3, t4, .Lbad_call
    la t4, fdi_spec_call_frames
    slli t5, t3, 6
    add t4, t4, t5
    sd ra, 0(t4)
    csrr t5, CSR_DRETURNPC
    sd t5, 8(t4)
    csrr t5, CSR_DFZRETURN
    sd t5, 16(t4)
    sd s11, 24(t4)
    sd sp, 32(t4)
    sd t1, 40(t4)
    csrr t5, CSR_DLCFG
    sd t5, 48(t4)
    addi t3, t3, 1
    sd t3, 0(t2)
    mv s11, t4
    /* DW_CFA_expression: the saved ra and s11 live at s11+0 and s11+24.
     * These descriptions allow diagnosis/unwinding; they do not clean the
     * side stack after a nonlocal transfer across this frame. */
    .cfi_escape 0x10, 0x01, 0x02, 0x8b, 0x00
    .cfi_escape 0x10, 0x1b, 0x02, 0x8b, 0x18
#if FDI_SPEC_DIAGNOSTIC
    la t2, fdi_spec_wrapper_hits
    slli t3, t1, 3
    add t2, t2, t3
    ld t3, 0(t2)
    addi t3, t3, 1
    sd t3, 0(t2)
#endif
    /* Open data access for this call and restore the enclosing call's state
     * afterwards. Bound endpoints are immutable after initialization. */
    li t2, 0xb
    csrw CSR_DLCFG, t2
    /* FDICALL.JR ra,t0 uses the current custom3 opcode. */
    .word DASICS_FDICALL_JR_RA_T0
    ld t0, 32(s11)
    bne t0, sp, .Lbad_call
    la t0, fdi_spec_call_depth
    ld t1, 0(t0)
    beqz t1, .Lbad_call
    addi t1, t1, -1
    la t2, fdi_spec_call_frames
    slli t3, t1, 6
    add t2, t2, t3
    bne t2, s11, .Lbad_call
    sd t1, 0(t0)
    ld t0, 48(s11)
    csrw CSR_DLCFG, t0
    ld t0, 8(s11)
    csrw CSR_DRETURNPC, t0
    ld t0, 16(s11)
    csrw CSR_DFZRETURN, t0
    ld ra, 0(s11)
    ld s11, 24(s11)
    .cfi_restore ra
    .cfi_restore s11
    ret
.Lbad_call:
    li a0, 6
    tail fdi_spec_panic
    .cfi_endproc
    .size fdi_spec_call, .-fdi_spec_call

    .balign 8
    .global fdi_spec_trap_entry
    .type fdi_spec_trap_entry, @function
fdi_spec_trap_entry:
    /* U-mode DASICS faults may interrupt any integer or FP instruction.
     * Preserve caller-saved FP registers and fcsr as well as the ABI-preserved
     * state before trusted C or the kernel can run. */
    addi sp, sp, -FDI_SPEC_TRAP_FRAME_SIZE
    sd ra, 0(sp)
    sd t0, 32(sp)
    addi t0, sp, FDI_SPEC_TRAP_FRAME_SIZE
    sd t0, 8(sp)
    sd gp, 16(sp)
    sd tp, 24(sp)
    sd t1, 40(sp)
    sd t2, 48(sp)
    sd s0, 56(sp)
    sd s1, 64(sp)
    sd a0, 72(sp)
    sd a1, 80(sp)
    sd a2, 88(sp)
    sd a3, 96(sp)
    sd a4, 104(sp)
    sd a5, 112(sp)
    sd a6, 120(sp)
    sd a7, 128(sp)
    sd s2, 136(sp)
    sd s3, 144(sp)
    sd s4, 152(sp)
    sd s5, 160(sp)
    sd s6, 168(sp)
    sd s7, 176(sp)
    sd s8, 184(sp)
    sd s9, 192(sp)
    sd s10, 200(sp)
    sd s11, 208(sp)
    sd t3, 216(sp)
    sd t4, 224(sp)
    sd t5, 232(sp)
    sd t6, 240(sp)
    fsd f0, 256(sp)
    fsd f1, 264(sp)
    fsd f2, 272(sp)
    fsd f3, 280(sp)
    fsd f4, 288(sp)
    fsd f5, 296(sp)
    fsd f6, 304(sp)
    fsd f7, 312(sp)
    fsd f8, 320(sp)
    fsd f9, 328(sp)
    fsd f10, 336(sp)
    fsd f11, 344(sp)
    fsd f12, 352(sp)
    fsd f13, 360(sp)
    fsd f14, 368(sp)
    fsd f15, 376(sp)
    fsd f16, 384(sp)
    fsd f17, 392(sp)
    fsd f18, 400(sp)
    fsd f19, 408(sp)
    fsd f20, 416(sp)
    fsd f21, 424(sp)
    fsd f22, 432(sp)
    fsd f23, 440(sp)
    fsd f24, 448(sp)
    fsd f25, 456(sp)
    fsd f26, 464(sp)
    fsd f27, 472(sp)
    fsd f28, 480(sp)
    fsd f29, 488(sp)
    fsd f30, 496(sp)
    fsd f31, 504(sp)
    frcsr t0
    sd t0, 512(sp)
    csrr t0, CSR_UEPC
    sd t0, 520(sp)
    csrr t0, CSR_USCRATCH
    sd t0, 528(sp)
    csrr t0, CSR_DRETURNPC
    sd t0, 536(sp)
    csrr t0, CSR_DFZRETURN
    sd t0, 544(sp)
    csrr t0, CSR_USTATUS
    sd t0, 552(sp)
    mv a0, sp
    call fdi_spec_trap_dispatch
    ld t0, 520(sp)
    csrw CSR_UEPC, t0
    ld t0, 528(sp)
    csrw CSR_USCRATCH, t0
    ld t0, 536(sp)
    csrw CSR_DRETURNPC, t0
    ld t0, 544(sp)
    csrw CSR_DFZRETURN, t0
    ld t0, 512(sp)
    fscsr t0
    fld f0, 256(sp)
    fld f1, 264(sp)
    fld f2, 272(sp)
    fld f3, 280(sp)
    fld f4, 288(sp)
    fld f5, 296(sp)
    fld f6, 304(sp)
    fld f7, 312(sp)
    fld f8, 320(sp)
    fld f9, 328(sp)
    fld f10, 336(sp)
    fld f11, 344(sp)
    fld f12, 352(sp)
    fld f13, 360(sp)
    fld f14, 368(sp)
    fld f15, 376(sp)
    fld f16, 384(sp)
    fld f17, 392(sp)
    fld f18, 400(sp)
    fld f19, 408(sp)
    fld f20, 416(sp)
    fld f21, 424(sp)
    fld f22, 432(sp)
    fld f23, 440(sp)
    fld f24, 448(sp)
    fld f25, 456(sp)
    fld f26, 464(sp)
    fld f27, 472(sp)
    fld f28, 480(sp)
    fld f29, 488(sp)
    fld f30, 496(sp)
    fld f31, 504(sp)
    ld ra, 0(sp)
    ld gp, 16(sp)
    ld tp, 24(sp)
    ld t0, 32(sp)
    ld t1, 40(sp)
    ld t2, 48(sp)
    ld s0, 56(sp)
    ld s1, 64(sp)
    ld a0, 72(sp)
    ld a1, 80(sp)
    ld a2, 88(sp)
    ld a3, 96(sp)
    ld a4, 104(sp)
    ld a5, 112(sp)
    ld a6, 120(sp)
    ld a7, 128(sp)
    ld s2, 136(sp)
    ld s3, 144(sp)
    ld s4, 152(sp)
    ld s5, 160(sp)
    ld s6, 168(sp)
    ld s7, 176(sp)
    ld s8, 184(sp)
    ld s9, 192(sp)
    ld s10, 200(sp)
    ld s11, 208(sp)
    ld t3, 216(sp)
    ld t4, 224(sp)
    ld t5, 232(sp)
    ld t6, 240(sp)
    addi sp, sp, FDI_SPEC_TRAP_FRAME_SIZE
    uret
    .size fdi_spec_trap_entry, .-fdi_spec_trap_entry

    .section .ulibtext.fdi_services,"ax",@progbits
    .macro UNTRUSTED_SERVICE name, service
    .balign 8
    .global \name
    .type \name, @function
\name:
    li t0, \service
    tail fdi_spec_maincall
    .size \name, .-\name
    .endm
    UNTRUSTED_SERVICE __wrap___cxa_atexit, FDI_SPEC_SERVICE_ATEXIT
    UNTRUSTED_SERVICE __wrap___cxa_finalize, FDI_SPEC_SERVICE_FINALIZE
    UNTRUSTED_SERVICE __wrap_exit, FDI_SPEC_SERVICE_EXIT
    UNTRUSTED_SERVICE __wrap__exit, FDI_SPEC_SERVICE_DIRECT_EXIT

    /* Exception entry from a library must never bypass the side-stack return
     * path. This fixed maincall reports the unsupported transfer and exits. */
    .macro UNTRUSTED_EH name, operation
    .balign 8
    .global \name
    .type \name, @function
\name:
    li a0, \operation
    li t0, FDI_SPEC_SERVICE_EH_UNSUPPORTED
    tail fdi_spec_maincall
    .size \name, .-\name
    .endm
    UNTRUSTED_EH __wrap___cxa_throw, FDI_SPEC_EH_THROW
    UNTRUSTED_EH __wrap___cxa_rethrow, FDI_SPEC_EH_RETHROW
    UNTRUSTED_EH __wrap__Unwind_Resume, FDI_SPEC_EH_RESUME
    UNTRUSTED_EH __wrap___cxa_begin_catch, FDI_SPEC_EH_BEGIN_CATCH
    UNTRUSTED_EH __wrap___cxa_end_catch, FDI_SPEC_EH_END_CATCH
    .option pop
    .section .note.GNU-stack,"",@progbits
