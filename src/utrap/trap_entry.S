#include <asm/asm.h>
#include <ucsr.h>
#include <asm/offset.h>

.section .text
.macro SAVE_CONTEXT
    addi sp, sp, -OFFSET_SIZE
    sd t0, OFFSET_TRAP_T0(sp)
    addi t0, sp, OFFSET_SIZE
    sd t0, OFFSET_TRAP_SP(sp)
    sd ra, OFFSET_TRAP_RA(sp)
    sd t1, OFFSET_TRAP_T1(sp)
    sd t2, OFFSET_TRAP_T2(sp)
    sd t3, OFFSET_TRAP_T3(sp)
    sd t4, OFFSET_TRAP_T4(sp)
    sd t5, OFFSET_TRAP_T5(sp)
    sd t6, OFFSET_TRAP_T6(sp)
    sd a0, OFFSET_TRAP_A0(sp)
    sd a1, OFFSET_TRAP_A1(sp)
    sd a2, OFFSET_TRAP_A2(sp)
    sd a3, OFFSET_TRAP_A3(sp)
    sd a4, OFFSET_TRAP_A4(sp)
    sd a5, OFFSET_TRAP_A5(sp)
    sd a6, OFFSET_TRAP_A6(sp)
    sd a7, OFFSET_TRAP_A7(sp)
    sd gp, OFFSET_TRAP_GP(sp)
    sd tp, OFFSET_TRAP_TP(sp)
    sd s0, OFFSET_TRAP_S0(sp)
    sd s1, OFFSET_TRAP_S1(sp)
    sd s2, OFFSET_TRAP_S2(sp)
    sd s3, OFFSET_TRAP_S3(sp)
    sd s4, OFFSET_TRAP_S4(sp)
    sd s5, OFFSET_TRAP_S5(sp)
    sd s6, OFFSET_TRAP_S6(sp)
    sd s7, OFFSET_TRAP_S7(sp)
    sd s8, OFFSET_TRAP_S8(sp)
    sd s9, OFFSET_TRAP_S9(sp)
    sd s10, OFFSET_TRAP_S10(sp)
    sd s11, OFFSET_TRAP_S11(sp)
    fsd f0, (OFFSET_TRAP_FPR + 0)(sp)
    fsd f1, (OFFSET_TRAP_FPR + 8)(sp)
    fsd f2, (OFFSET_TRAP_FPR + 16)(sp)
    fsd f3, (OFFSET_TRAP_FPR + 24)(sp)
    fsd f4, (OFFSET_TRAP_FPR + 32)(sp)
    fsd f5, (OFFSET_TRAP_FPR + 40)(sp)
    fsd f6, (OFFSET_TRAP_FPR + 48)(sp)
    fsd f7, (OFFSET_TRAP_FPR + 56)(sp)
    fsd f8, (OFFSET_TRAP_FPR + 64)(sp)
    fsd f9, (OFFSET_TRAP_FPR + 72)(sp)
    fsd f10, (OFFSET_TRAP_FPR + 80)(sp)
    fsd f11, (OFFSET_TRAP_FPR + 88)(sp)
    fsd f12, (OFFSET_TRAP_FPR + 96)(sp)
    fsd f13, (OFFSET_TRAP_FPR + 104)(sp)
    fsd f14, (OFFSET_TRAP_FPR + 112)(sp)
    fsd f15, (OFFSET_TRAP_FPR + 120)(sp)
    fsd f16, (OFFSET_TRAP_FPR + 128)(sp)
    fsd f17, (OFFSET_TRAP_FPR + 136)(sp)
    fsd f18, (OFFSET_TRAP_FPR + 144)(sp)
    fsd f19, (OFFSET_TRAP_FPR + 152)(sp)
    fsd f20, (OFFSET_TRAP_FPR + 160)(sp)
    fsd f21, (OFFSET_TRAP_FPR + 168)(sp)
    fsd f22, (OFFSET_TRAP_FPR + 176)(sp)
    fsd f23, (OFFSET_TRAP_FPR + 184)(sp)
    fsd f24, (OFFSET_TRAP_FPR + 192)(sp)
    fsd f25, (OFFSET_TRAP_FPR + 200)(sp)
    fsd f26, (OFFSET_TRAP_FPR + 208)(sp)
    fsd f27, (OFFSET_TRAP_FPR + 216)(sp)
    fsd f28, (OFFSET_TRAP_FPR + 224)(sp)
    fsd f29, (OFFSET_TRAP_FPR + 232)(sp)
    fsd f30, (OFFSET_TRAP_FPR + 240)(sp)
    fsd f31, (OFFSET_TRAP_FPR + 248)(sp)
    frcsr t0
    sd t0, OFFSET_TRAP_FCSR(sp)
    csrr t0, CSR_UEPC
    sd t0, OFFSET_TRAP_UEPC(sp)
    csrr t0, CSR_UTVAL
    sd t0, OFFSET_TRAP_UTVAL(sp)
    csrr t0, CSR_UCAUSE
    sd t0, OFFSET_TRAP_UCAUSE(sp)
    csrr t0, CSR_DRETURNPC
    sd t0, OFFSET_TRAP_RETURNPC(sp)
    csrr t0, CSR_DFZRETURN
    sd t0, OFFSET_TRAP_FZRETURNPC(sp)
    csrr t0, CSR_USCRATCH
    sd t0, OFFSET_TRAP_USCRATCH(sp)
    csrr t0, CSR_USTATUS
    sd t0, OFFSET_TRAP_USTATUS(sp)
    csrr t0, CSR_DFREASON
    sd t0, OFFSET_TRAP_DFREASON(sp)
    sd zero, OFFSET_TRAP_SYSCALL_ACTION(sp)
.endm

.macro RESTORE_CONTEXT
    ld t0, OFFSET_TRAP_UEPC(sp)
    csrw CSR_UEPC, t0
    ld t0, OFFSET_TRAP_UTVAL(sp)
    csrw CSR_UTVAL, t0
    ld t0, OFFSET_TRAP_UCAUSE(sp)
    csrw CSR_UCAUSE, t0
    ld t0, OFFSET_TRAP_RETURNPC(sp)
    csrw CSR_DRETURNPC, t0
    ld t0, OFFSET_TRAP_FZRETURNPC(sp)
    csrw CSR_DFZRETURN, t0
    ld t0, OFFSET_TRAP_USCRATCH(sp)
    csrw CSR_USCRATCH, t0
    ld t0, OFFSET_TRAP_USTATUS(sp)
    csrw CSR_USTATUS, t0
    ld t0, OFFSET_TRAP_DFREASON(sp)
    csrw CSR_DFREASON, t0
    ld t0, OFFSET_TRAP_FCSR(sp)
    fscsr t0
    fld f0, (OFFSET_TRAP_FPR + 0)(sp)
    fld f1, (OFFSET_TRAP_FPR + 8)(sp)
    fld f2, (OFFSET_TRAP_FPR + 16)(sp)
    fld f3, (OFFSET_TRAP_FPR + 24)(sp)
    fld f4, (OFFSET_TRAP_FPR + 32)(sp)
    fld f5, (OFFSET_TRAP_FPR + 40)(sp)
    fld f6, (OFFSET_TRAP_FPR + 48)(sp)
    fld f7, (OFFSET_TRAP_FPR + 56)(sp)
    fld f8, (OFFSET_TRAP_FPR + 64)(sp)
    fld f9, (OFFSET_TRAP_FPR + 72)(sp)
    fld f10, (OFFSET_TRAP_FPR + 80)(sp)
    fld f11, (OFFSET_TRAP_FPR + 88)(sp)
    fld f12, (OFFSET_TRAP_FPR + 96)(sp)
    fld f13, (OFFSET_TRAP_FPR + 104)(sp)
    fld f14, (OFFSET_TRAP_FPR + 112)(sp)
    fld f15, (OFFSET_TRAP_FPR + 120)(sp)
    fld f16, (OFFSET_TRAP_FPR + 128)(sp)
    fld f17, (OFFSET_TRAP_FPR + 136)(sp)
    fld f18, (OFFSET_TRAP_FPR + 144)(sp)
    fld f19, (OFFSET_TRAP_FPR + 152)(sp)
    fld f20, (OFFSET_TRAP_FPR + 160)(sp)
    fld f21, (OFFSET_TRAP_FPR + 168)(sp)
    fld f22, (OFFSET_TRAP_FPR + 176)(sp)
    fld f23, (OFFSET_TRAP_FPR + 184)(sp)
    fld f24, (OFFSET_TRAP_FPR + 192)(sp)
    fld f25, (OFFSET_TRAP_FPR + 200)(sp)
    fld f26, (OFFSET_TRAP_FPR + 208)(sp)
    fld f27, (OFFSET_TRAP_FPR + 216)(sp)
    fld f28, (OFFSET_TRAP_FPR + 224)(sp)
    fld f29, (OFFSET_TRAP_FPR + 232)(sp)
    fld f30, (OFFSET_TRAP_FPR + 240)(sp)
    fld f31, (OFFSET_TRAP_FPR + 248)(sp)
    ld ra, OFFSET_TRAP_RA(sp)
    ld t0, OFFSET_TRAP_T0(sp)
    ld t1, OFFSET_TRAP_T1(sp)
    ld t2, OFFSET_TRAP_T2(sp)
    ld t3, OFFSET_TRAP_T3(sp)
    ld t4, OFFSET_TRAP_T4(sp)
    ld t5, OFFSET_TRAP_T5(sp)
    ld t6, OFFSET_TRAP_T6(sp)
    ld a0, OFFSET_TRAP_A0(sp)
    ld a1, OFFSET_TRAP_A1(sp)
    ld a2, OFFSET_TRAP_A2(sp)
    ld a3, OFFSET_TRAP_A3(sp)
    ld a4, OFFSET_TRAP_A4(sp)
    ld a5, OFFSET_TRAP_A5(sp)
    ld a6, OFFSET_TRAP_A6(sp)
    ld a7, OFFSET_TRAP_A7(sp)
    ld gp, OFFSET_TRAP_GP(sp)
    ld tp, OFFSET_TRAP_TP(sp)
    ld s0, OFFSET_TRAP_S0(sp)
    ld s1, OFFSET_TRAP_S1(sp)
    ld s2, OFFSET_TRAP_S2(sp)
    ld s3, OFFSET_TRAP_S3(sp)
    ld s4, OFFSET_TRAP_S4(sp)
    ld s5, OFFSET_TRAP_S5(sp)
    ld s6, OFFSET_TRAP_S6(sp)
    ld s7, OFFSET_TRAP_S7(sp)
    ld s8, OFFSET_TRAP_S8(sp)
    ld s9, OFFSET_TRAP_S9(sp)
    ld s10, OFFSET_TRAP_S10(sp)
    ld s11, OFFSET_TRAP_S11(sp)
    ld sp, OFFSET_TRAP_SP(sp)
.endm

ENTRY(__dasics_uret)
    ld t0, OFFSET_TRAP_SYSCALL_ACTION(sp)
    li t1, DASICS_TRAP_PROXY_SYSCALL
    beq t0, t1, .Lproxy_syscall
    RESTORE_CONTEXT
    uret
.Lproxy_syscall:
    /* Run the approved syscall from trusted text after restoring its complete
     * architectural context. Preserve the kernel's a0 result through uret. */
    RESTORE_CONTEXT
    ecall
    uret
END(__dasics_uret)

ENTRY(dasics_ufault_entry)
    SAVE_CONTEXT
    mv a0, sp
#ifdef DASICS_NGINX_MODE1
    ld t0, OFFSET_TRAP_DFREASON(sp)
    li t1, DFR_ECALL_DASICS_FAULT
    beq t0, t1, .Lnginx_ecall
#endif
    call dasics_ufault_handler
    j __dasics_uret
END(dasics_ufault_entry)

ENTRY(_setup_fault)
    SAVE_CONTEXT
    mv a0, sp
#ifdef DASICS_NGINX_MODE1
    ld t0, OFFSET_TRAP_DFREASON(sp)
    li t1, DFR_ECALL_DASICS_FAULT
    beq t0, t1, .Lnginx_ecall
    call dasics_start_fault
    j __dasics_uret
END(_setup_fault)

.Lnginx_ecall:
    call handle_DasicsUEcallFault
    j __dasics_uret
#else
    call dasics_start_fault
    j __dasics_uret
END(_setup_fault)
#endif

/* A normal C-callable raw proxy; trap dispatch instead uses the restore-first
 * path above so its public register context remains authoritative. */
ENTRY(invoke_syscall)
    mv t0, a0
    ld a7, OFFSET_TRAP_A7(t0)
    ld a6, OFFSET_TRAP_A6(t0)
    ld a5, OFFSET_TRAP_A5(t0)
    ld a4, OFFSET_TRAP_A4(t0)
    ld a3, OFFSET_TRAP_A3(t0)
    ld a2, OFFSET_TRAP_A2(t0)
    ld a1, OFFSET_TRAP_A1(t0)
    ld a0, OFFSET_TRAP_A0(t0)
    ecall
    ret
END(invoke_syscall)

.section .note.GNU-stack,"",@progbits
