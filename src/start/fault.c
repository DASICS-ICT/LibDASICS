#include <dasics_start.h>
#ifdef DASICS_NGINX_MODE1
#include <utrap.h>
#include <ucsr.h>
#include <dasics_stdio.h>

void dasics_start_fault(struct ucontext_trap * regs)
{

    uint64_t dasics_return_pc = regs->return_pc;            // DasicsReturnPC
    uint64_t dasics_free_zone_return_pc = regs->freezone_return_pc;  // DasicsFreeZoneReturnPC
    uint64_t dasics_dfreason = regs->dfreason;             // DasicsDFreason


    switch (dasics_dfreason)
    {
    case DFR_JUMP_DASICS_FAULT:
        /* code */
        if (dasics_return_pc != regs->utval)
        {
            regs->return_pc = regs->utval;
        }


        if (dasics_free_zone_return_pc != regs->utval)
        {
            regs->freezone_return_pc = regs->utval;
        }
        break;

    case DFR_ECALL_DASICS_FAULT:
        // jump ecall
        regs->uepc += 4;
        regs->a0 = invoke_syscall(regs);

        //unreachable here
        break;

    default:
        dasics_printf("[ERROR]: Handle impossible UFAULT trap: %lx, addr:%lx \n", regs->ucause, regs->uepc);
        while(1);
        break;
    }

}



#else
#include <udasics.h>

/* Startup uses the same delegated-fault policy as the installed runtime.
 * The private dynamic-loader auxv protocol does not authorize jump recovery. */
void dasics_start_fault(struct ucontext_trap *regs)
{
    dasics_ufault_handler(regs);
}
#endif
