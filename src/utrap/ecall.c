#include <utrap.h>
#include <usyscall.h>
#include <errno.h>

int handle_DasicsUEcallFault(struct ucontext_trap *regs)
{
    const volatile unsigned char *pc = (const void *)regs->uepc;
    if (regs->ucause != EXC_DASICS_UCHECK_FAULT ||
        regs->dfreason != DFR_ECALL_DASICS_FAULT ||
        pc[0] != 0x73 || pc[1] || pc[2] || pc[3]) return -1;
    regs->syscall_action = 0;
    regs->uepc += 4;
    if (regs->a7 >= __NR_syscalls) {
        regs->a0 = (reg_t)-ENOSYS;
        return 0;
    }
    ecall_check_t *policy = &syscall_check[regs->a7];
    ecall_check_handler check = policy->check ? policy->check : default_ecall_check_handler;
    int denied = check(regs->a0, regs->a1, regs->a2, regs->a3,
                       regs->a4, regs->a5, regs->a6, regs->a7);
    if (denied) {
        regs->a0 = policy->handle_error ? (reg_t)(long)policy->handle_error() : (reg_t)-EPERM;
    } else {
        regs->syscall_action = DASICS_TRAP_PROXY_SYSCALL;
    }
    return 0;
}
