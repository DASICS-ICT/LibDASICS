#include <hook.h>
#include <umaincall.h>

uint64_t dasics_func_pointer_hook(struct umaincall * regs)
{
    if (regs->a0 != DASICS_HOOK_FUNC_MAGIC) return 0;

    regs->t1 = regs->a1;
    regs->a0 = regs->a2;
    regs->a1 = regs->a3;
    regs->a2 = regs->a4;
    regs->a3 = regs->a5;
    regs->a4 = regs->a6;
    regs->a5 = regs->a7;
    return 1;
    
}