#include <utrap.h>

/* Do not turn an arbitrary faulting target into a permitted return address.
 * Registered recovery handlers can explicitly update the saved return state. */
int handle_DasicsUFetchFault(struct ucontext_trap *regs)
{
    (void)regs;
    return -1;
}
