#include <utrap.h>
#include <dasics_stdio.h>

/*
 * Default ZIMT tag-fault policy. Tests and runtimes can override this with
 * register_utag_fault_handler().
 */
int handle_DasicsUTagFault(struct ucontext_trap * regs)
{
    dasics_printf("[DASICS_EXCEPTION]: Tag fault: 0x%lx pc: 0x%lx\n",
                  regs->utval, regs->uepc);
    return -1;
}
