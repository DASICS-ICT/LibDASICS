#ifndef _INCLUDE_ASM_OFFSET_H
#define _INCLUDE_ASM_OFFSET_H

/* Existing context prefixes retain their public field offsets. */
#define OFFSET_TRAP_UEPC             0
#define OFFSET_TRAP_UTVAL            8
#define OFFSET_TRAP_UCAUSE           16
#define OFFSET_TRAP_RA               24
#define OFFSET_TRAP_SP               32
#define OFFSET_TRAP_T0               40
#define OFFSET_TRAP_T1               48
#define OFFSET_TRAP_T2               56
#define OFFSET_TRAP_T3               64
#define OFFSET_TRAP_T4               72
#define OFFSET_TRAP_T5               80
#define OFFSET_TRAP_T6               88
#define OFFSET_TRAP_A0               96
#define OFFSET_TRAP_A1               104
#define OFFSET_TRAP_A2               112
#define OFFSET_TRAP_A3               120
#define OFFSET_TRAP_A4               128
#define OFFSET_TRAP_A5               136
#define OFFSET_TRAP_A6               144
#define OFFSET_TRAP_A7               152
#define OFFSET_TRAP_GP               160
#define OFFSET_TRAP_TP               168
#define OFFSET_TRAP_S0               176
#define OFFSET_TRAP_S1               184
#define OFFSET_TRAP_S2               192
#define OFFSET_TRAP_S3               200
#define OFFSET_TRAP_S4               208
#define OFFSET_TRAP_S5               216
#define OFFSET_TRAP_S6               224
#define OFFSET_TRAP_S7               232
#define OFFSET_TRAP_S8               240
#define OFFSET_TRAP_S9               248
#define OFFSET_TRAP_S10              256
#define OFFSET_TRAP_S11              264
#define OFFSET_TRAP_FPR 272
#define OFFSET_TRAP_FCSR 528
#define OFFSET_TRAP_RETURNPC 536
#define OFFSET_TRAP_FZRETURNPC 544
#define OFFSET_TRAP_USCRATCH 552
#define OFFSET_TRAP_USTATUS 560
#define OFFSET_TRAP_DFREASON 568
#define OFFSET_TRAP_SYSCALL_ACTION 576
#define OFFSET_SIZE 592
#define DASICS_TRAP_PROXY_SYSCALL 1

#define OFFSET_UMAINCALL_T0           0
#define OFFSET_UMAINCALL_T1           8
#define OFFSET_UMAINCALL_T3           16
#define OFFSET_UMAINCALL_RA           24
#define OFFSET_UMAINCALL_A0           32
#define OFFSET_UMAINCALL_A1           40
#define OFFSET_UMAINCALL_A2           48
#define OFFSET_UMAINCALL_A3           56
#define OFFSET_UMAINCALL_A4           64
#define OFFSET_UMAINCALL_A5           72
#define OFFSET_UMAINCALL_A6           80
#define OFFSET_UMAINCALL_A7           88
#define OFFSET_UMAINCALL_SP           96
#define OFFSET_UMAINCALL_FPR 104
#define OFFSET_UMAINCALL_FCSR 360
#define OFFSET_UMAINCALL_RETURNPC 368
#define OFFSET_UMAINCALL_FZRETURNPC 376
#define OFFSET_UMAINCALL_VARARGS 400
#define OFFSET_UMAINCALL 448

#endif
