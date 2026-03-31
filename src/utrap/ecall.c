#include <utrap.h>
#include <usyscall.h>
#include <fit.h>

int handle_DasicsUEcallFault(struct ucontext_trap * regs)
{
  // Do uepc ++ first
  regs->uepc += 4;  

  // Check whether the function closure is allowed to call the syscall
  if (fit_check_syscall(regs->a7) != 0)
  {
    // Invalid syscall
    printf("[DASICS EXCEPTION] Invalid syscall %lu at %lx, cancel this syscall\n", regs->a7, regs->uepc - 4);
    return -1;  // Return -1 to terminate the program
  }

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