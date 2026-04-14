#include <utrap.h>
#include <dasics_stdio.h>
#include <ucsr.h>

/* Handle DASICS Fetch Fault */ 
int handle_DasicsUFetchFault(struct ucontext_trap * regs)
{
    uint64_t dasics_return_pc = csr_read(dretpc);
    uint64_t dasics_free_zone_return_pc = csr_read(dretpcactz);
    uint32_t insn;

    dasics_printf("[DASICS_EXCEPTION]: Fetch fault: 0x%lx, pc: 0x%lx\n",
                  regs->utval, regs->uepc);

    /*
     * Returning from an untrusted compartment may legitimately fault when
     * jumping back to dretpc / dretpcactz. Preserve the old behavior for
     * those return targets so FIT transitions can complete normally.
     */
    if (regs->utval == dasics_return_pc || regs->utval == dasics_free_zone_return_pc) {
        asm("fence.i");
        return 0;
    }

    /* Skip the offending jump/call instruction, like ldst handlers do. */
    insn = *((uint32_t *)regs->uepc);
    if ((insn & 0x3) == 0x3)
        regs->uepc += 4;
    else
        regs->uepc += 2;

    return 0;
}
