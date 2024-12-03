#include <dasics_start.h>
#include <utrap.h>
#include <ucsr.h>
#include <dasics_stdio.h>

void dasics_start_fault(struct ucontext_trap * regs)
{

    uint64_t dasics_return_pc = csr_read(0x8b1);            // DasicsReturnPC
    uint64_t dasics_free_zone_return_pc = csr_read(0x8b2);  // DasicsFreeZoneReturnPC
    uint64_t dasics_dfreason = csr_read(0x8b3);             // DasicsDFreason
    

    switch (dasics_dfreason)
    {
    case DFR_JUMP_DASICS_FAULT:
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



