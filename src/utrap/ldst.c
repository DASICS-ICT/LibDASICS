#include <utrap.h>

/* Bound-cache misses are retried by the dispatcher only after a complete
 * permission check. A registered handler may provide another explicit policy. */
int handle_DasicsULoadFault(struct ucontext_trap *regs)
{
    (void)regs;
    return -1;
}

int handle_DasicsUStoreFault(struct ucontext_trap *regs)
{
    (void)regs;
    return -1;
}
