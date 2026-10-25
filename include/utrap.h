#ifndef _INCLUDE_UTRAP_H
#define _INCLUDE_UTRAP_H

#include <stdint.h>
#include <stddef.h>
#include "dasics_isa.h"
#include "asm/offset.h"

typedef unsigned long reg_t;

/* The original 160-byte prefix is unchanged. Registered recovery handlers
 * may update saved registers, return PCs, and uepc before the common restore.
 * Only syscall_action=PROXY requests a real ECALL after full restoration. */
struct ucontext_trap {
    reg_t uepc;
    reg_t utval;
    reg_t ucause;
    reg_t ra;
    reg_t sp;
    reg_t t0;
    reg_t t1;
    reg_t t2;
    reg_t t3;
    reg_t t4;
    reg_t t5;
    reg_t t6;
    reg_t a0;
    reg_t a1;
    reg_t a2;
    reg_t a3;
    reg_t a4;
    reg_t a5;
    reg_t a6;
    reg_t a7;
    reg_t gp;
    reg_t tp;
    reg_t s0;
    reg_t s1;
    reg_t s2;
    reg_t s3;
    reg_t s4;
    reg_t s5;
    reg_t s6;
    reg_t s7;
    reg_t s8;
    reg_t s9;
    reg_t s10;
    reg_t s11;
    reg_t fpr[32];
    reg_t fcsr, return_pc, freezone_return_pc, uscratch, ustatus, dfreason;
    reg_t syscall_action, reserved;
};

typedef int (*utrap_handler)(struct ucontext_trap *regs);
int handle_DasicsUFetchFault(struct ucontext_trap *regs);
int handle_DasicsULoadFault(struct ucontext_trap *regs);
int handle_DasicsUStoreFault(struct ucontext_trap *regs);
int handle_DasicsUEcallFault(struct ucontext_trap *regs);
extern long invoke_syscall(struct ucontext_trap *regs);

#endif
