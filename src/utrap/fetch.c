#include <utrap.h>
#include <dasics_stdio.h>
#include <ucsr.h>
#include <stdio.h>

/* Handle DASICS Fetch Fault */ 
int handle_DasicsUFetchFault(struct ucontext_trap * regs)
{
    uint64_t dasics_return_pc = csr_read(dretpc);
    uint64_t dasics_free_zone_return_pc = csr_read(dretpcactz);

    if (dasics_return_pc != regs->utval)
    {
        csr_write(dretpc, regs->utval);
        asm("fence.i");
        return 0;
    }
        

    if (dasics_free_zone_return_pc != regs->utval)
    {
        csr_write(dretpcactz, regs->utval);
        asm("fence.i");
        return 0;
    }
        

    return 0;

}
