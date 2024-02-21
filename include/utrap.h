#ifndef _INCLUDE_UTRAP_H
#define _INCLUDE_UTRAP_H

/*  
 * This is used for dasics utrap handler, include all struct and handler
 */

#include <stdint.h>
#include <stddef.h>

typedef uint64_t reg_t;


/**
 * The register which needed to be saved by the callee will be save properly 
 * by the standered RISCV ABI
 */
struct ucontext_trap
{
	reg_t uepc;
	reg_t utval;
	reg_t ucause;

    /* Saved main processor registers.*/
	reg_t ra;
	reg_t sp;
	reg_t gp;
	reg_t tp;
	reg_t t0;
	reg_t t1;
	reg_t t2;
	reg_t a0;
	reg_t a1;
	reg_t a2;
	reg_t a3;
	reg_t a4;
	reg_t a5;
	reg_t a6;
	reg_t a7;
	reg_t t3;
	reg_t t4;
	reg_t t5;
	reg_t t6;
};


#define OFFSET_TRAP_MEMBER(t) offsetof(struct ucontext_trap, t)

#define OFFSET_TRAP_UEPC    OFFSET_TRAP_MEMBER(uepc)
#define OFFSET_TRAP_UTVAL   OFFSET_TRAP_MEMBER(utval)
#define OFFSET_TRAP_UCAUSE  OFFSET_TRAP_MEMBER(ucause)
#define OFFSET_TRAP_RA      OFFSET_TRAP_MEMBER(ra)
#define OFFSET_TRAP_SP      OFFSET_TRAP_MEMBER(sp)
#define OFFSET_TRAP_GP      OFFSET_TRAP_MEMBER(gp)
#define OFFSET_TRAP_TP      OFFSET_TRAP_MEMBER(tp)
#define OFFSET_TRAP_T0      OFFSET_TRAP_MEMBER(t0)
#define OFFSET_TRAP_T1      OFFSET_TRAP_MEMBER(t1)
#define OFFSET_TRAP_T2      OFFSET_TRAP_MEMBER(t2)
#define OFFSET_TRAP_T3      OFFSET_TRAP_MEMBER(t3)
#define OFFSET_TRAP_T4      OFFSET_TRAP_MEMBER(t4)
#define OFFSET_TRAP_T5      OFFSET_TRAP_MEMBER(t5)
#define OFFSET_TRAP_T6      OFFSET_TRAP_MEMBER(t6)
#define OFFSET_TRAP_A0      OFFSET_TRAP_MEMBER(a0)
#define OFFSET_TRAP_A1      OFFSET_TRAP_MEMBER(a1)
#define OFFSET_TRAP_A2      OFFSET_TRAP_MEMBER(a2)
#define OFFSET_TRAP_A3      OFFSET_TRAP_MEMBER(a3)
#define OFFSET_TRAP_A4      OFFSET_TRAP_MEMBER(a4)
#define OFFSET_TRAP_A5      OFFSET_TRAP_MEMBER(a5)
#define OFFSET_TRAP_A6      OFFSET_TRAP_MEMBER(a6)

#define OFFSET_SIZE         sizeof(struct ucontext_trap)

/* DASICS exceptions */
#define EXC_DASICS_UFETCH_FAULT     24
#define EXC_DASICS_SFETCH_FAULT     25
#define EXC_DASICS_ULOAD_FAULT      26
#define EXC_DASICS_SLOAD_FAULT      27
#define EXC_DASICS_USTORE_FAULT     28
#define EXC_DASICS_SSTORE_FAULT     29
#define EXC_DASICS_UECALL_FAULT     30
#define EXC_DASICS_SECALL_FAULT     31


/*
 * Three types of U-exception types
 */
int handle_DasicsUFetchFault(struct ucontext_trap * r_regs);
int handle_DasicsULoadFault(struct ucontext_trap * r_regs);
int handle_DasicsUStoreFault(struct ucontext_trap * r_regs);
int handle_DasicsUEcallFault(struct ucontext_trap * r_regs);





#endif