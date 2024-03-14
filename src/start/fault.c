#include <dasics_start.h>
#include <utrap.h>
#include <ucsr.h>
#include <dasics_stdio.h>

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

void dasics_start_fault(struct ucontext_trap * regs)
{

    uint64_t dasics_return_pc = csr_read(0x8b1);            // DasicsReturnPC
    uint64_t dasics_free_zone_return_pc = csr_read(0x8b2);  // DasicsFreeZoneReturnPC
    

    switch (regs->ucause)
    {
    case EXC_DASICS_UFETCH_FAULT:
        /* code */
        if (dasics_return_pc != regs->utval)
        {
            csr_write(0x8b1, regs->utval);
        }
            

        if (dasics_free_zone_return_pc != regs->utval)
        {
            csr_write(0x8b2, regs->utval);
        }
        break;

    case EXC_DASICS_UECALL_FAULT:
        regs->a0 = invoke_syscall(regs->a7, \
                                    regs->a0, \
                                    regs->a1, \
                                    regs->a2, \
                                    regs->a3, \
                                    regs->a4, \
                                    regs->a5, \
                                    regs->a6);


        // jump ecall
        regs->uepc += 4;
        break;

    default:
        dasics_printf("[ERROR]: Handle impossible UFAULT trap: %lx, addr:%lx \n", regs->ucause, regs->uepc);
        while(1);
        break;
    }

}



