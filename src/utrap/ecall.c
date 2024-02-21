#include <utrap.h>


static long invoke_syscall(long sysno, long arg0, long arg1, long arg2,
                           long arg3, long arg4, long arg5, long arg6)
{
    register long a7 asm("a7") = sysno;
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a2 asm("a2") = arg2;
    register long a3 asm("a3") = arg3;
    register long a4 asm("a4") = arg4;
    register long a5 asm("a5") = arg5;
    register long a6 asm("a6") = arg6;

    asm volatile("ecall"                      \
                 : "+r"(a0)                   \
                 : "r"(a1), "r"(a2), "r"(a3), \
                   "r"(a4), "r"(a5), "r"(a6), \
                   "r"(a7)                    \
                 : "memory");

    return a0;
}



int handle_DasicsUEcallFault(struct ucontext_trap * r_regs)
{
  r_regs->a0 = invoke_syscall(r_regs->a7, \
                              r_regs->a0, \
                              r_regs->a1, \
                              r_regs->a2, \
                              r_regs->a3, \
                              r_regs->a4, \
                              r_regs->a5, \
                              r_regs->a6);


  // jump ecall
  r_regs->uepc += 4;

  return 0;

}