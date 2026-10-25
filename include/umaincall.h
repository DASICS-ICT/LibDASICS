#ifndef _INCLUDE_UMAINCALL_H
#define _INCLUDE_UMAINCALL_H

/* Used for the maincall */

#include <stdint.h>
#include <stddef.h>
#include <list.h>

typedef unsigned long reg_t;


/**
 * auipc	t3,0x2
 * ld	t3,-1072(t3) # 2020 <free>
 * jalr	t1,t3
 * nop
 * 
 * t1 is used to calculate the indx of dynamic call
 * t3 is used to sure the target is umain_call
 * 
 * a0 is used to variable length parameters
 * ra is used to change jump target
 */

/* The original 104-byte prefix stays stable. The trailing register argument
 * area ends at the caller's original sp for RISC-V variadic forwarding. */
struct umaincall {
    reg_t t0;
    reg_t t1;
    reg_t t3;
    reg_t ra;
    reg_t a0;
    reg_t a1;
    reg_t a2;
    reg_t a3;
    reg_t a4;
    reg_t a5;
    reg_t a6;
    reg_t a7;
    reg_t sp;
    reg_t fpr[32];
    reg_t fcsr, return_pc, freezone_return_pc;
    reg_t reserved[2];
    reg_t vararg_registers[6];
};

#include "asm/offset.h"

#define NOP 0x00000013

extern uint64_t umaincall_helper;

typedef void * (*umaincall_check_handler)(unsigned long, \
                                    unsigned long, \
                                    unsigned long, \
                                    unsigned long, \
                                    unsigned long, \
                                    unsigned long, \
                                    unsigned long, \
                                    unsigned long);

struct umaincall_handler
{
    list_head list;
    uint64_t maincallnum;
    umaincall_check_handler handler;
};


// Open maincall
int _open_maincall();

int dasics_dynamic_call(struct umaincall * CallContext);
void dasics_dynamic_return(struct umaincall * CallContext);

void init_maincall_handler();

#endif