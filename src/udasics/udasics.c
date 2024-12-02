#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
// #include <machine/syscall.h>
#include "udasics.h"
#include "uthash.h"
#include <utrap.h>
#include <dasics_stdio.h>
#include <umaincall.h>

uint64_t umaincall_helper;

utrap_handler udasics_ecall_fault_handler = handle_DasicsUEcallFault;
utrap_handler udasics_load_fault_handler  = handle_DasicsULoadFault;
utrap_handler udasics_store_fault_handler = handle_DasicsUStoreFault;
utrap_handler udasics_fetch_fault_handler = handle_DasicsUFetchFault;


#define BOUND_REG_READ(hi,lo,idx)   \
        case idx:  \
            lo = csr_read(0x890 + idx * 2);  \
            hi = csr_read(0x891 + idx * 2);  \
            break;

#define BOUND_REG_WRITE(hi,lo,idx)   \
        case idx:  \
            csr_write(0x890 + idx * 2, lo);  \
            csr_write(0x891 + idx * 2, hi);  \
            break;

#define CONCAT(OP) BOUND_REG_##OP

#define LIBBOUND_LOOKUP(HI,LO,IDX,OP) \
        switch (IDX) \
        {               \
            CONCAT(OP)(HI,LO,0);  \
            CONCAT(OP)(HI,LO,1);  \
            CONCAT(OP)(HI,LO,2);  \
            CONCAT(OP)(HI,LO,3);  \
            CONCAT(OP)(HI,LO,4);  \
            CONCAT(OP)(HI,LO,5);  \
            CONCAT(OP)(HI,LO,6);  \
            CONCAT(OP)(HI,LO,7);  \
            CONCAT(OP)(HI,LO,8);  \
            CONCAT(OP)(HI,LO,9);  \
            CONCAT(OP)(HI,LO,10); \
            CONCAT(OP)(HI,LO,11); \
            CONCAT(OP)(HI,LO,12); \
            CONCAT(OP)(HI,LO,13); \
            CONCAT(OP)(HI,LO,14); \
            CONCAT(OP)(HI,LO,15); \
            default: \
                printf("\x1b[31m%s\x1b[0m","[DASICS]Error: out of libound register range\n"); \
        }

typedef struct {
    uint64_t lo;
    uint64_t hi;
} bound_t;

typedef struct {
    int handle;  /* key */
    int priv;
    bound_t bound;
    UT_hash_handle hh;
} hashed_bound_t;

hashed_bound_t *bounds_table = NULL;
static int available_handle = 0;  // FIXME: Currently we ignore int overflow conditions

int dlibcfg_handle_map[DASICS_LIBCFG_WIDTH] = {-1};

void register_udasics(uint64_t funcptr) 
{
    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
    int32_t step = 4;
    // Set random seed
    srand(2023);

    // Write OS-allocated bounds to hash table
    for (int32_t idx = 0; idx < max_cfgs; ++idx) {
        uint64_t curr_cfg = (libcfg >> (idx * step)) & DASICS_LIBCFG_MASK;

        // Found allocated config
        if ((curr_cfg & DASICS_LIBCFG_V) != 0) {
            uint64_t hi, lo;
            LIBBOUND_LOOKUP(hi, lo, idx, READ);
            hashed_bound_t *entry = (hashed_bound_t *)malloc(sizeof(hashed_bound_t));
            entry->bound.hi = hi;
            entry->bound.lo = lo;
            entry->priv = curr_cfg;
            entry->handle = available_handle++;
            HASH_ADD_INT(bounds_table, handle, entry);
            dlibcfg_handle_map[idx] = entry->handle;
        }
    }

    // Set maincall & ufault handler
    umaincall_helper = (funcptr != 0) ? funcptr : (uint64_t) dasics_umaincall_helper;
    csr_write(0x8b0, (uint64_t)dasics_umaincall);
    csr_write(0x005, (uint64_t)dasics_ufault_entry);
}

void unregister_udasics(void) 
{
    // csr_write(0x8b0, 0);
    // csr_write(0x005, 0);

    // Free bounds hash table
    hashed_bound_t *current, *temp;
    HASH_ITER(hh, bounds_table, current, temp) {
        HASH_DEL(bounds_table, current);
        free(current);
    }
}

void resgister_uecall_fault_handler(utrap_handler ecall_fault_handler)
{
    if (ecall_fault_handler != NULL)
        udasics_ecall_fault_handler = ecall_fault_handler;

}
void resgister_uload_fault_handler(utrap_handler load_fault_handler)
{
    if (load_fault_handler != NULL)
        udasics_load_fault_handler = load_fault_handler;
}

void resgister_ustore_fault_handler(utrap_handler store_fault_handler)
{
    if (store_fault_handler != NULL)
        udasics_store_fault_handler = store_fault_handler;
}

void resgister_ufetch_fault_handler(utrap_handler fetch_fault_handler)
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
            LIBBOUND_LOOKUP(bounds[items].hi, bounds[items].lo, idx, READ);
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


void dasics_umaincall_helper(struct umaincall * regs, ...)
{
    // uint64_t dasics_return_pc = csr_read(0x8b1);            // DasicsReturnPC
    // uint64_t dasics_free_zone_return_pc = csr_read(0x8b2);  // DasicsFreeZoneReturnPC
    // Judge This is a dynamic call
    if (dasics_dynamic_call(regs)) return;

    uint64_t retval = 0;

    va_list args;
    UmaincallTypes type = (UmaincallTypes)regs->a0;

    va_start(args, type);

    switch (type)
    {
        case Umaincall_PRINT: {
            const char *format = va_arg(args, const char *);
            retval = vprintf(format, args);
        }
        break;

        default:
            printf("\x1b[33m Warning: Invalid umaincall number %d!\n\x1b[0m", type); //could not use printf in kernel
            break;
    }

    regs->t1 = regs->ra;
    regs->a0 = retval;
    // csr_write(0x8b1, dasics_return_pc);             // DasicsReturnPC
    // csr_write(0x8b2, dasics_free_zone_return_pc);   // DasicsFreeZoneReturnPC

    va_end(args);
}

static int dasics_oldest_victim(void) {
    uint64_t dlaging0 = csr_read(0x881);  // DasicsLibAging0
    uint64_t dlaging1 = csr_read(0x882);  // DasicsLibAging1
    const uint64_t aging_width = 8;

    int victim = 0;
    uint8_t oldest = 0xffu;  // Smaller value is older

    for (int i = 0; i < DASICS_LIBCFG_WIDTH; ++i) {
        uint8_t aging_val;
        if (i < DASICS_LIBCFG_WIDTH / 2) {
            aging_val = (uint8_t)(dlaging0 >> (i * aging_width));
        } else {
            aging_val = (uint8_t)(dlaging1 >> ((i - DASICS_LIBCFG_WIDTH / 2) * aging_width));
        }
        if (aging_val <= oldest) {
            victim = i;
            oldest = aging_val;
        }
    }

    return victim;
}

/**
 * Check whether the bounds hash table contains the corresponding entry
 * If so, load the entry to dlibcfg and dlibbounds
 * @param utval faulting memory address
 * @param is_read this operation is read or write
 * @return -1 if cannot find hash table entry; newly inserted dlibcfg idx if found
 */
static int dasics_ldst_checker(uint64_t utval, int is_read)
{
    int valid_perm = DASICS_LIBCFG_V | (is_read ? DASICS_LIBCFG_R : DASICS_LIBCFG_W);
    uint64_t libcfg = csr_read(0x880);   // DasicsLibCfg
    int step = 4;

    // Iterate bounds table to find matching bounds
    hashed_bound_t *current, *temp;
    HASH_ITER(hh, bounds_table, current, temp) {
        if (current->bound.lo <= utval && utval < current->bound.hi && \
            (current->priv & valid_perm) == valid_perm) {
            // Find the matching bound, thus replace one libcfg & libbound with it
            // int victim = dasics_oldest_victim();
            int victim = rand() % DASICS_LIBCFG_WIDTH;
            LIBBOUND_LOOKUP(current->bound.hi, current->bound.lo, victim, WRITE);

            // Write config
            libcfg &= ~(DASICS_LIBCFG_MASK << (victim * step));
            libcfg |= ((uint64_t)current->priv) << (victim * step);
            csr_write(0x880, libcfg);   // DasicsLibCfg

             // Fill dlibcsr map with new handle
            dlibcfg_handle_map[victim] = current->handle;
            
            return victim;
        }
    }

    return -1;
}

void dasics_ufault_handler(struct ucontext_trap * regs)
{
    // Save some registers that should be saved by callees
    int error;
    int csr_idx;
    switch (regs->ucause)
    {
    case EXC_DASICS_UFETCH_FAULT:
        error = udasics_fetch_fault_handler(regs);
        break;
    
    case EXC_DASICS_ULOAD_FAULT:
        // csr_idx = dasics_ldst_checker(regs->utval, 1);

        // if (0 <= csr_idx && csr_idx < DASICS_LIBCFG_WIDTH) {
        //     uint64_t hi, lo;
        //     LIBBOUND_LOOKUP(hi, lo, csr_idx, READ);
        //     dasics_printf("[DASICS EXCEPTION]Info: dasics uload fault OK! new csr idx is %d, lo = %#lx, hi = %#lx\n", \
        //         csr_idx, lo, hi);
        //     return;
        // }

        error = udasics_load_fault_handler(regs);
        break;

    case EXC_DASICS_USTORE_FAULT:
        // csr_idx = dasics_ldst_checker(regs->utval, 0);

        // if (0 <= csr_idx && csr_idx < DASICS_LIBCFG_WIDTH) {
        //     uint64_t hi, lo;
        //     LIBBOUND_LOOKUP(hi, lo, csr_idx, READ);
        //     dasics_printf("[DASICS EXCEPTION]Info: dasics ustore fault OK! new csr idx is %d, lo = %#lx, hi = %#lx\n", \
        //         csr_idx, lo, hi);
        //     return;
        // }      

        error = udasics_store_fault_handler(regs);
        break;
    
    case EXC_DASICS_UECALL_FAULT:
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
    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
    int32_t step = 4;

    lo = align8down(lo);
    hi = align8up(hi);

    // Insert new bound information to hash table
    hashed_bound_t *entry = (hashed_bound_t *)malloc(sizeof(hashed_bound_t));
    entry->bound.hi = hi;
    entry->bound.lo = lo;
    entry->priv = (cfg & DASICS_LIBCFG_MASK) | DASICS_LIBCFG_V;
    entry->handle = available_handle++;
    HASH_ADD_INT(bounds_table, handle, entry);

    // Find a proper libcfg for the newly allocated bound
    int32_t victim = 0;
    for (; victim < max_cfgs; ++victim) {
        uint64_t curr_cfg = (libcfg >> (victim * step)) & DASICS_LIBCFG_MASK;

        // Found available config
        if ((curr_cfg & DASICS_LIBCFG_V) == 0) {
            break;
        }
    }

    // Kick out the oldest victim if we cannot find one available place
    if (victim == max_cfgs) {
        // victim = dasics_oldest_victim();
        victim = rand() % DASICS_LIBCFG_WIDTH;
    }

    // Write libbound
    LIBBOUND_LOOKUP(hi, lo, victim, WRITE);

    // Write config
    libcfg &= ~(DASICS_LIBCFG_MASK << (victim * step));
    libcfg |= ((uint64_t)entry->priv) << (victim * step);
    csr_write(0x880, libcfg);   // DasicsLibCfg

    // // Write init aging value
    // uint64_t aging_width = 8;
    // if (victim < DASICS_LIBCFG_WIDTH / 2) {
    //     uint64_t dlaging = csr_read(0x881);
    //     dlaging |= 0xfful << (victim * aging_width);
    //     csr_write(0x881, dlaging);
    // } else {
    //     uint64_t dlaging = csr_read(0x882);
    //     dlaging |= 0xfful << ((victim - DASICS_LIBCFG_WIDTH / 2) * aging_width);
    //     csr_write(0x882, dlaging);
    // }

    // Fill dlibcsr map with new handle
    dlibcfg_handle_map[victim] = entry->handle;

    return entry->handle;
}

int32_t dasics_libcfg_free(int32_t handle) {
    if (handle < 0) {
        return -1;
    }

    // Lookup hashed table firstly
    hashed_bound_t *entry;
    HASH_FIND_INT(bounds_table, &handle, entry);

    if (NULL == entry) {
        return -1;
    }

    // Erase hash map entry
    HASH_DEL(bounds_table, entry);
    free(entry);

    // Check if the target bound exists in dasics CSRs
    int32_t idx = 0;
    int32_t step = 4;
    for (; idx < DASICS_LIBCFG_WIDTH; ++idx) {
        if (dlibcfg_handle_map[idx] == handle) {
            uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
            libcfg &= ~(DASICS_LIBCFG_V << (idx * step));
            csr_write(0x880, libcfg);   // DasicsLibCfg
            dlibcfg_handle_map[idx] = -1;
            break;
        }
    }

    return 0;
}

int32_t dasics_libcfg_free_all()
{
    int32_t idx = 0;

    for (; idx < DASICS_LIBCFG_WIDTH; ++idx) {
        if (dlibcfg_handle_map[idx] != -1) {
            dasics_libcfg_free(dlibcfg_handle_map[idx]);
        }
    }   
    return 0;
}

uint32_t dasics_libcfg_get(int32_t handle) {
    hashed_bound_t *entry;
    HASH_FIND_INT(bounds_table, &handle, entry);

    if (NULL == entry) {
        return 0;
    } else {
        return entry->priv;
    }
}

int32_t dasics_jumpcfg_alloc(uint64_t lo, uint64_t hi)
{
    uint64_t jumpcfg = csr_read(0x8c8);    // DasicsJumpCfg
    int32_t max_cfgs = DASICS_JUMPCFG_WIDTH;
    int32_t step = 16;

    for (int32_t idx = 0; idx < max_cfgs; ++idx) {
        uint64_t curr_cfg = (jumpcfg >> (idx * step)) & DASICS_JUMPCFG_MASK;
        if ((curr_cfg & DASICS_JUMPCFG_V) == 0) // found available cfg
        {
            // Write DASICS jump boundary CSRs
            switch (idx) {
                case 0:
                    csr_write(0x8c0, lo);  // DasicsJumpBound0Lo
                    csr_write(0x8c1, hi);  // DasicsJumpBound0Hi
                    break;
                case 1:
                    csr_write(0x8c2, lo);  // DasicsJumpBound1Lo
                    csr_write(0x8c3, hi);  // DasicsJumpBound1Hi
                    break;
                case 2:
                    csr_write(0x8c4, lo);  // DasicsJumpBound2Lo
                    csr_write(0x8c5, hi);  // DasicsJumpBound2Hi
                    break;
                case 3:
                    csr_write(0x8c6, lo);  // DasicsJumpBound3Lo
                    csr_write(0x8c7, hi);  // DasicsJumpBound3Hi
                    break;
                default:
                    break;
            }

            jumpcfg &= ~(DASICS_JUMPCFG_MASK << (idx * step));
            jumpcfg |= DASICS_JUMPCFG_V << (idx * step);
            csr_write(0x8c8, jumpcfg); // DasicsJumpCfg

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
    uint64_t jumpcfg = csr_read(0x8c8);    // DasicsJumpCfg
    jumpcfg &= ~(DASICS_JUMPCFG_V << (idx * step));
    csr_write(0x8c8, jumpcfg); // DasicsJumpCfg
    return 0;
}


void dasics_print_cfg_register(int32_t handle)
{
	printf("DASICS uLib CFG Registers: handle:%x  config: %x \n",handle,dasics_libcfg_get(handle));
}


