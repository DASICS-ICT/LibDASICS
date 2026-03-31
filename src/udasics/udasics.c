#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
// #include <machine/syscall.h>
#include "udasics.h"
#include <utrap.h>
#include <dasics_stdio.h>
#include <umaincall.h>
#include <fit.h>
#include <mimalloc.h>

uint64_t umaincall_helper;

utrap_handler udasics_ecall_fault_handler = handle_DasicsUEcallFault;
utrap_handler udasics_load_fault_handler  = handle_DasicsULoadFault;
utrap_handler udasics_store_fault_handler = handle_DasicsUStoreFault;
utrap_handler udasics_fetch_fault_handler = handle_DasicsUFetchFault;


#define LIBBOUND_READ(HI, LO, IDX) \
        switch (IDX) { \
            case 0:  LO = csr_read(dlbound0);  HI = csr_read(dlbound1);  break; \
            case 1:  LO = csr_read(dlbound2);  HI = csr_read(dlbound3);  break; \
            case 2:  LO = csr_read(dlbound4);  HI = csr_read(dlbound5);  break; \
            case 3:  LO = csr_read(dlbound6);  HI = csr_read(dlbound7);  break; \
            case 4:  LO = csr_read(dlbound8);  HI = csr_read(dlbound9);  break; \
            case 5:  LO = csr_read(dlbound10); HI = csr_read(dlbound11); break; \
            case 6:  LO = csr_read(dlbound12); HI = csr_read(dlbound13); break; \
            case 7:  LO = csr_read(dlbound14); HI = csr_read(dlbound15); break; \
            case 8:  LO = csr_read(dlbound16); HI = csr_read(dlbound17); break; \
            case 9:  LO = csr_read(dlbound18); HI = csr_read(dlbound19); break; \
            case 10: LO = csr_read(dlbound20); HI = csr_read(dlbound21); break; \
            case 11: LO = csr_read(dlbound22); HI = csr_read(dlbound23); break; \
            case 12: LO = csr_read(dlbound24); HI = csr_read(dlbound25); break; \
            case 13: LO = csr_read(dlbound26); HI = csr_read(dlbound27); break; \
            case 14: LO = csr_read(dlbound28); HI = csr_read(dlbound29); break; \
            case 15: LO = csr_read(dlbound30); HI = csr_read(dlbound31); break; \
            default: printf("\x1b[31m%s\x1b[0m","[DASICS]Error: out of libound register range\n"); \
        }

#define LIBBOUND_WRITE(HI, LO, IDX) \
        switch (IDX) { \
            case 0:  csr_write(dlbound0,  LO); csr_write(dlbound1,  HI); break; \
            case 1:  csr_write(dlbound2,  LO); csr_write(dlbound3,  HI); break; \
            case 2:  csr_write(dlbound4,  LO); csr_write(dlbound5,  HI); break; \
            case 3:  csr_write(dlbound6,  LO); csr_write(dlbound7,  HI); break; \
            case 4:  csr_write(dlbound8,  LO); csr_write(dlbound9,  HI); break; \
            case 5:  csr_write(dlbound10, LO); csr_write(dlbound11, HI); break; \
            case 6:  csr_write(dlbound12, LO); csr_write(dlbound13, HI); break; \
            case 7:  csr_write(dlbound14, LO); csr_write(dlbound15, HI); break; \
            case 8:  csr_write(dlbound16, LO); csr_write(dlbound17, HI); break; \
            case 9:  csr_write(dlbound18, LO); csr_write(dlbound19, HI); break; \
            case 10: csr_write(dlbound20, LO); csr_write(dlbound21, HI); break; \
            case 11: csr_write(dlbound22, LO); csr_write(dlbound23, HI); break; \
            case 12: csr_write(dlbound24, LO); csr_write(dlbound25, HI); break; \
            case 13: csr_write(dlbound26, LO); csr_write(dlbound27, HI); break; \
            case 14: csr_write(dlbound28, LO); csr_write(dlbound29, HI); break; \
            case 15: csr_write(dlbound30, LO); csr_write(dlbound31, HI); break; \
            default: printf("\x1b[31m%s\x1b[0m","[DASICS]Error: out of libound register range\n"); \
        }

typedef struct {
    uint64_t lo;
    uint64_t hi;
} bound_t;

void register_udasics(uint64_t funcptr) 
{
    // Set maincall & ufault handler
    umaincall_helper = (funcptr != 0) ? funcptr : (uint64_t) dasics_umaincall_helper;
    csr_write(dmaincall, (uint64_t)dasics_umaincall);
    csr_write(utvec, (uint64_t)dasics_ufault_entry);
}

void unregister_udasics(void) 
{
    // csr_write(dmaincall, 0);
    // csr_write(utvec, 0);
}

void register_uecall_fault_handler(utrap_handler ecall_fault_handler)
{
    if (ecall_fault_handler != NULL)
        udasics_ecall_fault_handler = ecall_fault_handler;

}
void register_uload_fault_handler(utrap_handler load_fault_handler)
{
    if (load_fault_handler != NULL)
        udasics_load_fault_handler = load_fault_handler;
}

void register_ustore_fault_handler(utrap_handler store_fault_handler)
{
    if (store_fault_handler != NULL)
        udasics_store_fault_handler = store_fault_handler;
}

void register_ufetch_fault_handler(utrap_handler fetch_fault_handler)
{
    if (fetch_fault_handler != NULL)
        udasics_fetch_fault_handler = fetch_fault_handler;
}


static int bound_coverage_cmp(const void *a, const void *b)
{
    const bound_t *_a = (const bound_t *)a;
    const bound_t *_b = (const bound_t *)b;
    return (_a->lo < _b->lo) ? -1 : 1;
}

static int dasics_bound_checker(uint64_t lo, uint64_t hi, int perm)
{
    // In fact, this is a bound coverage problem for [lo, hi]
    bound_t bounds[DASICS_LIBCFG_WIDTH];
    int32_t idx, items = 0;
    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;

    // Fill bounds array with permission matched libbounds
    for (idx = 0; idx < max_cfgs; ++idx) {
        uint32_t cfg = dasics_libcfg_get(idx);
        if ((cfg & DASICS_LIBCFG_V) == 0) {
            continue;
        }
        else if ((cfg & (perm | DASICS_LIBCFG_V)) != DASICS_LIBCFG_V) {
            // Permission matched, add this libbound to bound list
            LIBBOUND_READ(bounds[items].hi, bounds[items].lo, idx);
            items++;
        }
    }

    // Based on the lower bound, sort bounds array in an increasing order
    qsort(bounds, items, sizeof(bound_t), bound_coverage_cmp);

    // Calculate bound coverage via greedy algorithm
    for (idx = 0; idx < items; ++idx) {
        if (bounds[idx].lo <= lo + 1 && lo <= bounds[idx].hi) {
            lo = bounds[idx].hi;
        }
        else if (bounds[idx].hi < lo) {
            continue;
        }
        else {
            break;
        }
    }

    return hi <= lo;
}


uint64_t dasics_umaincall_helper(UmaincallTypes type, ...)
{
    // uint64_t dasics_return_pc = csr_read(dretpc);
    // uint64_t dasics_free_zone_return_pc = csr_read(dretpcactz);
    // Judge This is a dynamic call
    uint64_t retval = 0;

    // Check whether the maincall is allowed to be called by the function closure
    uint64_t maincall_pc = (uint64_t)(__builtin_return_address(0)) - 4;  // 4 is the length of jal
    if (fit_check_maincall(type) != 0) {
        printf("[DASICS MAINCALL] Invalid maincall %d at %lx, cancel this maincall\n", \
            type, maincall_pc);
        return -1;
    }

    // Perform the maincall
    va_list args;
    va_start(args, type);

    switch (type)
    {
        case Umaincall_PRINT: {
            const char *format = va_arg(args, const char *);
            retval = vprintf(format, args);
        }
        break;

        case Umaincall_MALLOC: {
            size_t size = va_arg(args, size_t);
            void *p = malloc(size);
            retval = (uint64_t)p;
            if (!p)
                break;
            /* Scheme B: on first alloc for this compartment, add heap bound and set heap_alloc_done.
             * Obtains the compartment directly from the compartment stack. */
            compartment_t *entry = fit_get_current_compartment();
            if (!entry)
                break;
            if (entry->heap_alloc_done)
                break;
            void *seg = NULL;
            size_t area_size = 0;
            if (mi_get_mem_area_dasics(entry->library_id, entry->closure_id, &seg, &area_size) != 0 || !seg || area_size == 0)
                break;
            uint64_t lo = (uint64_t)seg;
            uint64_t hi = lo + area_size;  /* dasics_libcfg_alloc expects exclusive hi */
            int32_t h = dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_W, lo, hi);
            if (h < 0)
                break;
            if (entry->mem_bounds_num >= FIT_MEM_BOUNDS_MAX)
                break;
            entry->mem_bounds[entry->mem_bounds_num].perm = DASICS_LIBCFG_R | DASICS_LIBCFG_W;
            entry->mem_bounds[entry->mem_bounds_num].lo = lo;
            entry->mem_bounds[entry->mem_bounds_num].hi = hi - 1;  /* store inclusive hi in mem_bounds */
            entry->mem_bounds[entry->mem_bounds_num].handle = h;
            entry->mem_bounds_num++;
            entry->heap_alloc_done = 1;
        }
        break;

        case Umaincall_PGRANT: {
            void *func_target = va_arg(args, void *);
            const fit_bounds_t *perms = va_arg(args, const fit_bounds_t *);
            size_t num = va_arg(args, size_t);
            size_t valist_size = va_arg(args, size_t);
            unsigned times = (unsigned)va_arg(args, unsigned int);
            /* Look up the target compartment by function pointer. */
            compartment_t *target = fit_find(func_target);
            if (!target) {
                retval = (uint64_t)-1;
                break;
            }
            if (do_permission_grant(target, perms, num, valist_size, times) != 0)
                retval = (uint64_t)-1;
        }
        break;

        case Umaincall_TRANS: {
            void *func = va_arg(args, void *);
            retval = do_transition(func, args);
        }
        break;

        case Umaincall_FREE: {
            // FIXME: Add pointer authority check!
            void *p = va_arg(args, void *);
            if (p)
                free(p);
        }
        break;

        default:
            printf("\x1b[33m Warning: Invalid umaincall number %d!\n\x1b[0m", type);
            break;
    }

    // csr_write(dretpc, dasics_return_pc);
    // csr_write(dretpcactz, dasics_free_zone_return_pc);

    va_end(args);
    return retval;
}


void dasics_ufault_handler(struct ucontext_trap * regs)
{
    int error;

    uint64_t dasics_dfreason = csr_read(dfreason);

    switch (dasics_dfreason)
    {
    case DFR_JUMP_DASICS_FAULT:
        error = udasics_fetch_fault_handler(regs);
        break;
    
    case DFR_LOAD_DASICS_FAULT:
        error = udasics_load_fault_handler(regs);
        break;

    case DFR_STORE_DASICS_FAULT:
        error = udasics_store_fault_handler(regs);
        break;
    
    case DFR_ECALL_DASICS_FAULT:
        error = udasics_ecall_fault_handler(regs);
        break;
        
    default:
        dasics_printf("[ERROR] unhandle ufault: 0x%lx\n", regs->ucause);
        exit(1);
    }

    if (error == -1)
        exit(1);    
}

int32_t dasics_libcfg_alloc(uint64_t cfg, uint64_t lo, uint64_t hi) {
    uint64_t libcfg = csr_read(dlcfg);
    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
    int32_t step = 4;

    lo = align8down(lo);
    hi = align8up(hi);

    for (int32_t idx = 0; idx < max_cfgs; ++idx) {
        uint64_t curr_cfg = (libcfg >> (idx * step)) & DASICS_LIBCFG_MASK;

        // Found available config
        if ((curr_cfg & DASICS_LIBCFG_V) == 0) {
            LIBBOUND_WRITE(hi, lo, idx);

            libcfg &= ~(DASICS_LIBCFG_MASK << (idx * step));
            libcfg |= (((cfg & DASICS_LIBCFG_MASK) | DASICS_LIBCFG_V)) << (idx * step);
            csr_write(dlcfg, libcfg);

            return idx;
        }
    }

    return -1;
}

int32_t dasics_libcfg_free(int32_t idx) {
    if (idx < 0 || idx >= DASICS_LIBCFG_WIDTH) return -1;

    int32_t step = 4;
    uint64_t libcfg = csr_read(dlcfg);
    libcfg &= ~(DASICS_LIBCFG_V << (idx * step));
    csr_write(dlcfg, libcfg);
    return 0;
}

int32_t dasics_libcfg_free_all()
{
    csr_write(dlcfg, 0);
    return 0;
}

uint32_t dasics_libcfg_get(int32_t idx) {
    if (idx < 0 || idx >= DASICS_LIBCFG_WIDTH) return 0;

    int32_t step = 4;
    uint64_t libcfg = csr_read(dlcfg);
    return (libcfg >> (idx * step)) & DASICS_LIBCFG_MASK;
}

int32_t dasics_jumpcfg_alloc(uint64_t lo, uint64_t hi)
{
    uint64_t jumpcfg = csr_read(djcfg);
    int32_t max_cfgs = DASICS_JUMPCFG_WIDTH;
    int32_t step = 16;

    for (int32_t idx = 0; idx < max_cfgs; ++idx) {
        uint64_t curr_cfg = (jumpcfg >> (idx * step)) & DASICS_JUMPCFG_MASK;
        if ((curr_cfg & DASICS_JUMPCFG_V) == 0) // found available cfg
        {
            // Write DASICS jump boundary CSRs
            switch (idx) {
                case 0: csr_write(djbound0, lo); csr_write(djbound1, hi); break;
                case 1: csr_write(djbound2, lo); csr_write(djbound3, hi); break;
                case 2: csr_write(djbound4, lo); csr_write(djbound5, hi); break;
                case 3: csr_write(djbound6, lo); csr_write(djbound7, hi); break;
                default: break;
            }

            jumpcfg &= ~(DASICS_JUMPCFG_MASK << (idx * step));
            jumpcfg |= DASICS_JUMPCFG_V << (idx * step);
            csr_write(djcfg, jumpcfg);

            return idx;
        }
    }

    return -1;
}

int32_t dasics_jumpcfg_free(int32_t idx) {
    if (idx < 0 || idx >= DASICS_JUMPCFG_WIDTH) {
        return -1;
    }

    int32_t step = 16;
    uint64_t jumpcfg = csr_read(djcfg);
    jumpcfg &= ~(DASICS_JUMPCFG_V << (idx * step));
    csr_write(djcfg, jumpcfg);
    return 0;
}

int32_t dasics_jumpcfg_free_all(void) {
    csr_write(djcfg, 0);
    return 0;
}


void dasics_print_cfg_register(int32_t idx)
{
	printf("DASICS uLib CFG Registers: idx:%d  config: %x \n", idx, dasics_libcfg_get(idx));
}
