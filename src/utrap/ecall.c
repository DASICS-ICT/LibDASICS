#include <utrap.h>
#include <usyscall.h>


int handle_DasicsUEcallFault(struct ucontext_trap * regs)
{
  // Do uepc ++ first
  regs->uepc += 4;  

  syscall_check[regs->a7].check(
    regs->a0,
    regs->a1,
    regs->a2,
    regs->a3,
    regs->a4,
    regs->a5,
    regs->a6,
    regs->a7
  );

  // unreachable here

  return 0;

}