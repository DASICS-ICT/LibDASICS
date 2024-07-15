#ifndef _INCLUDE_USYSCALL_H
#define _INCLUDE_USYSCALL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <syscall.h>
#include <stdint.h>

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