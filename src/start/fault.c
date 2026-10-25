#include <dasics_start.h>
#include <udasics.h>

/* Startup uses the same delegated-fault policy as the installed runtime.
 * The private dynamic-loader auxv protocol does not authorize jump recovery. */
void dasics_start_fault(struct ucontext_trap *regs)
{
    dasics_ufault_handler(regs);
}
