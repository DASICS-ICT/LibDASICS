#include <utrap.h>
#ifdef DASICS_NGINX_MODE1
#include <dasics_stdio.h>
#include <ucsr.h>
#include <stdio.h>

/* Handle DASICS Fetch Fault */
int handle_DasicsUFetchFault(struct ucontext_trap * regs)
{
    // dasics_printf("[DASICS_EXCEPTION]: Fetch fault\n");


    uint64_t dasics_return_pc = regs->return_pc;            // DasicsReturnPC
    uint64_t dasics_free_zone_return_pc = regs->freezone_return_pc;  // DasicsFreeZoneReturnPC


    // dasics_printf("DASICS_EXCEPTION: utval: 0x%lx, record: 0x%lx\n", regs->utval, dasics_return_pc);



    if (dasics_return_pc != regs->utval)
    {
        regs->return_pc = regs->utval;
        asm("fence.i");
        return 0;
    }


    if (dasics_free_zone_return_pc != regs->utval)
    {
        regs->freezone_return_pc = regs->utval;
        asm("fence.i");
        return 0;
    }


    return 0;

}




#else

/* Do not turn an arbitrary faulting target into a permitted return address.
 * Registered recovery handlers can explicitly update the saved return state. */
int handle_DasicsUFetchFault(struct ucontext_trap *regs)
{
    (void)regs;
    return -1;
}
#endif
