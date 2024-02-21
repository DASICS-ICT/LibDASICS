#ifndef _INCLUDE_USYSCALL_H
#define _INCLUDE_USYSCALL_H

#define SYSCALL_ARGS  long sysno, long arg1, long arg2, \
        long arg3, long arg4, long arg5, long arg6

static inline long __attribute__((always_inline)) ulib_syscall(SYSCALL_ARGS) {
    register long a0 asm("a0") = arg1;
    register long a1 asm("a1") = arg2;
    register long a2 asm("a2") = arg3;
    register long a3 asm("a3") = arg4;
    register long a4 asm("a4") = arg5;
    register long a5 asm("a5") = arg6;
    register long a7 asm("a7") = sysno;

    asm volatile("ecall"                        \
                 : "+r"(a0)                     \
                 : "r"(a1), "r"(a2), "r"(a3),   \
                   "r"(a4), "r"(a5), "r"(a7)    \
                 : "memory");

    return a0;
}

#define ULIB_SYSCALL6(sysno, arg1, arg2, arg3, arg4, arg5, arg6) \
	ulib_syscall(sysno, (long)arg1, (long)arg2, (long)arg3, (long)arg4, (long)arg5, (long)arg6)
#define ULIB_SYSCALL4(sysno, arg1, arg2, arg3, arg4) \
	ULIB_SYSCALL6(sysno, arg1, arg2, arg3, arg4, 0, 0)
#define ULIB_SYSCALL3(sysno, arg1, arg2, arg3) \
	ULIB_SYSCALL6(sysno, arg1, arg2, arg3, 0, 0, 0)
#define ULIB_SYSCALL1(sysno, arg1) \
	ULIB_SYSCALL6(sysno, arg1, 0, 0, 0, 0, 0)


#endif