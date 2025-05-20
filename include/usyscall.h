#ifndef _INCLUDE_USYSCALL_H
#define _INCLUDE_USYSCALL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <syscall.h>
#include <stdint.h>

#define SYSCALL_ARGS long sysno, long arg1, long arg2, \
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
#define ULIB_SYSCALL5(sysno, arg1, arg2, arg3, arg4, arg5) \
    ULIB_SYSCALL6(sysno, arg1, arg2, arg3, arg4, arg5, 0)
#define ULIB_SYSCALL4(sysno, arg1, arg2, arg3, arg4) \
    ULIB_SYSCALL6(sysno, arg1, arg2, arg3, arg4, 0, 0)
#define ULIB_SYSCALL3(sysno, arg1, arg2, arg3) \
    ULIB_SYSCALL6(sysno, arg1, arg2, arg3, 0, 0, 0)
#define ULIB_SYSCALL2(sysno, arg1, arg2) \
    ULIB_SYSCALL6(sysno, arg1, arg2, 0, 0, 0, 0)
#define ULIB_SYSCALL1(sysno, arg1) \
    ULIB_SYSCALL6(sysno, arg1, 0, 0, 0, 0, 0)
#define ULIB_SYSCALL0(sysno) \
    ULIB_SYSCALL6(sysno, 0, 0, 0, 0, 0, 0)

typedef int (*ecall_check_handler)(unsigned long, \
                                    unsigned long, \
                                    unsigned long, \
                                    unsigned long, \
                                    unsigned long, \
                                    unsigned long, \
                                    unsigned long, \
                                    unsigned long);

typedef int (*ecall_error_handler)(void);
/*
 * The ecall check struct
 */
typedef struct ecall_check
{
    ecall_check_handler check;
    ecall_error_handler handle_error;
} ecall_check_t;

extern ecall_check_t syscall_check[__NR_syscalls];



int default_ecall_check_handler(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7);
int default_ecall_error_handler();
void init_syscall_check();

int register_syscall_check(int sysno, ecall_check_handler check_handler, ecall_error_handler error_handler);

#ifdef __cplusplus
}
#endif

#endif