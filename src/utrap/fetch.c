#include <utrap.h>
#include <dasics_stdio.h>
#include <ucsr.h>

/* Handle DASICS Fetch Fault */ 
int handle_DasicsUFetchFault(struct ucontext_trap * r_regs)
{
    dasics_printf("[DASICS_EXCEPTION]: Fetch fault\n");


    uint64_t dasicsReturnPC = csr_read(0x8b1);

    

    return 0;

}




