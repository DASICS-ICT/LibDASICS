#ifndef _INCLUDE_UMAINCALL_H
#define _INCLUDE_UMAINCALL_H

/* Used for the maincall */

#include <stdint.h>
#include <stddef.h>

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

struct umaincall
{
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
};


#define OFFSET_UMAINCALL_T1     (8*0)
#define OFFSET_UMAINCALL_T3     (8*1)
#define OFFSET_UMAINCALL_RA     (8*2)
#define OFFSET_UMAINCALL_A0     (8*3)
#define OFFSET_UMAINCALL_A1     (8*4)
#define OFFSET_UMAINCALL_A2     (8*5)
#define OFFSET_UMAINCALL_A3     (8*6)
#define OFFSET_UMAINCALL_A4     (8*7)
#define OFFSET_UMAINCALL_A5     (8*8)
#define OFFSET_UMAINCALL_A6     (8*9)
#define OFFSET_UMAINCALL_A7     (8*10)

#define OFFSET_UMAINCALL        sizeof(struct umaincall)



#endif