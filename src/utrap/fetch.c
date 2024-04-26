#include <utrap.h>
#include <dasics_stdio.h>
#include <ucsr.h>
#include <stdio.h>

/* Handle DASICS Fetch Fault */ 
int handle_DasicsUFetchFault(struct ucontext_trap * regs)
{
    dasics_printf("[DASICS_EXCEPTION]: Fetch fault, 0x%lx\n", regs->uepc);


    uint64_t dasics_return_pc = csr_read(0x8b4);            // DasicsReturnPC
    uint64_t dasics_free_zone_return_pc = csr_read(0x8b2);  // DasicsFreeZoneReturnPC


    dasics_printf("DASICS_EXCEPTION: utval: 0x%lx, record: 0x%lx\n", regs->utval, dasics_return_pc);
    dasics_printf("CSR_DJCFG: 0x%lx, CSR_DJBOUND0LO: 0x%lx, CSR_DJBOUND0L1: 0x%lx\n",
                csr_read(0x8c8), csr_read(0x8c0), csr_read(0x8c1));


    if (dasics_return_pc != regs->utval)
    {
        csr_write(0x8b4, regs->utval);
        return 0;
    }
        

    if (dasics_free_zone_return_pc != regs->utval)
    {
        csr_write(0x8b2, regs->utval);
        return 0;
    }
        

    return 0;

}




