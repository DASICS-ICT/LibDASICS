#ifndef _INCLUDE_ASM_OFFSET_H
#define _INCLUDE_ASM_OFFSET_H

/* Define all offset of asm */

/* Trap used */ 
#define OFFSET_TRAP_UEPC    (8*0)
#define OFFSET_TRAP_UTVAL   (8*1)
#define OFFSET_TRAP_UCAUSE  (8*2)
#define OFFSET_TRAP_RA      (8*3)
#define OFFSET_TRAP_SP      (8*4)
#define OFFSET_TRAP_T0      (8*5)
#define OFFSET_TRAP_T1      (8*6)
#define OFFSET_TRAP_T2      (8*7)
#define OFFSET_TRAP_T3      (8*8)
#define OFFSET_TRAP_T4      (8*9)
#define OFFSET_TRAP_T5      (8*10)
#define OFFSET_TRAP_T6      (8*11)
#define OFFSET_TRAP_A0      (8*12)
#define OFFSET_TRAP_A1      (8*13)
#define OFFSET_TRAP_A2      (8*14)
#define OFFSET_TRAP_A3      (8*15)
#define OFFSET_TRAP_A4      (8*16)
#define OFFSET_TRAP_A5      (8*17)
#define OFFSET_TRAP_A6      (8*18)
#define OFFSET_TRAP_A7      (8*19)


#define OFFSET_SIZE         (8*20)

/* maincall used */ 
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


#define OFFSET_UMAINCALL        (8*11)

#endif