#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#ifndef DASICS_NGINX_MODE1
#include <limits.h>
#include <stddef.h>
#include <dasics_bounds.h>
#endif
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

#ifdef DASICS_NGINX_MODE1
uint64_t libcfg = 0;
uint64_t jumpcfg = 0;
#endif

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
#ifdef DASICS_NGINX_MODE1
    int active;
    int map_idx;
#endif
    bound_t bound;
    UT_hash_handle hh;
} hashed_bound_t;

hashed_bound_t *bounds_table = NULL;
static int available_handle = 0;  // FIXME: Currently we ignore int overflow conditions

#ifdef DASICS_NGINX_MODE1
int dlibcfg_handle_map[DASICS_LIBCFG_WIDTH] = {-1};

void register_udasics(uint64_t funcptr)
{
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
#else
int dlibcfg_handle_map[DASICS_LIBCFG_WIDTH] = {
    [0 ... DASICS_LIBCFG_WIDTH - 1] = -1
};

/* Registration owns the process-local FDI bank until unregister restores the
 * previous caller's state. These interfaces are not concurrent-thread APIs. */
static struct {
    uint64_t libcfg, jumpcfg, maincall, utvec;
    bound_t lib[DASICS_LIBCFG_WIDTH], jump[DASICS_JUMPCFG_WIDTH];
    int active;
} saved_state;

void register_udasics(uint64_t funcptr)
{
    if (!saved_state.active) {
        saved_state.libcfg = csr_read(CSR_DLCFG);
        saved_state.jumpcfg = csr_read(CSR_DJCFG);
        saved_state.maincall = csr_read(CSR_DMAINCALL);
        saved_state.utvec = csr_read(CSR_UTVEC);
        saved_state.jump[0].lo = csr_read(CSR_DJBOUND0LO);
        saved_state.jump[0].hi = csr_read(CSR_DJBOUND0HI);
        saved_state.jump[1].lo = csr_read(CSR_DJBOUND1LO);
        saved_state.jump[1].hi = csr_read(CSR_DJBOUND1HI);
        saved_state.jump[2].lo = csr_read(CSR_DJBOUND2LO);
        saved_state.jump[2].hi = csr_read(CSR_DJBOUND2HI);
        saved_state.jump[3].lo = csr_read(CSR_DJBOUND3LO);
        saved_state.jump[3].hi = csr_read(CSR_DJBOUND3HI);
        srand(2023);
        for (int idx = 0; idx < DASICS_LIBCFG_WIDTH; ++idx) {
            uint64_t lo, hi;
            unsigned int cfg = (saved_state.libcfg >> (idx * 4)) & DASICS_LIBCFG_MASK;
            LIBBOUND_LOOKUP(hi, lo, idx, READ);
            saved_state.lib[idx].lo = lo;
            saved_state.lib[idx].hi = hi;
            int previous_handle = dlibcfg_handle_map[idx];
            hashed_bound_t *previous = NULL;
            HASH_FIND_INT(bounds_table, &previous_handle, previous);
            dlibcfg_handle_map[idx] = -1;
            if (cfg & DASICS_LIBCFG_V) {
                if (previous && previous->bound.lo == lo && previous->bound.hi == hi &&
                    previous->priv == (int)cfg) {
                    dlibcfg_handle_map[idx] = previous_handle;
                    continue;
                }
                hashed_bound_t *entry = malloc(sizeof(*entry));
                if (!entry || available_handle == INT_MAX) exit(1);
                entry->bound.lo = lo;
                entry->bound.hi = hi;
                entry->priv = cfg;
                entry->handle = available_handle++;
                HASH_ADD_INT(bounds_table, handle, entry);
                dlibcfg_handle_map[idx] = entry->handle;
            }
        }
        saved_state.active = 1;
    }
    umaincall_helper = funcptr ? funcptr : (uint64_t)dasics_umaincall_helper;
    csr_write(CSR_DMAINCALL, (uint64_t)dasics_umaincall);
    csr_write(CSR_UTVEC, (uint64_t)dasics_ufault_entry);
}

void unregister_udasics(void)
{
    if (!saved_state.active) return;
    dasics_libcfg_free_all();
    csr_write(CSR_DJCFG, 0);
    for (int idx = 0; idx < DASICS_LIBCFG_WIDTH; ++idx)
        LIBBOUND_LOOKUP(saved_state.lib[idx].hi, saved_state.lib[idx].lo, idx, WRITE);
    csr_write(CSR_DJBOUND0LO, saved_state.jump[0].lo);
    csr_write(CSR_DJBOUND0HI, saved_state.jump[0].hi);
    csr_write(CSR_DJBOUND1LO, saved_state.jump[1].lo);
    csr_write(CSR_DJBOUND1HI, saved_state.jump[1].hi);
    csr_write(CSR_DJBOUND2LO, saved_state.jump[2].lo);
    csr_write(CSR_DJBOUND2HI, saved_state.jump[2].hi);
    csr_write(CSR_DJBOUND3LO, saved_state.jump[3].lo);
    csr_write(CSR_DJBOUND3HI, saved_state.jump[3].hi);
    csr_write(CSR_DLCFG, saved_state.libcfg);
    csr_write(CSR_DJCFG, saved_state.jumpcfg);
    csr_write(CSR_DMAINCALL, saved_state.maincall);
    csr_write(CSR_UTVEC, saved_state.utvec);
    saved_state.active = 0;
#endif
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


#ifdef DASICS_NGINX_MODE1
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


uint64_t dasics_umaincall_helper(UmaincallTypes type, ...)
{
    // uint64_t dasics_return_pc = csr_read(0x8b1);            // DasicsReturnPC
    // uint64_t dasics_free_zone_return_pc = csr_read(0x8b2);  // DasicsFreeZoneReturnPC
    // Judge This is a dynamic call
    uint64_t retval = 0;

    va_list args;
    va_start(args, type);

    switch (type)
    {
        case Umaincall_PRINT: {
            const char *format = va_arg(args, const char *);
#else
void dasics_umaincall_helper(struct umaincall * regs, ...)
{
    // uint64_t dasics_return_pc = csr_read(0x8b1);            // DasicsReturnPC
    // uint64_t dasics_free_zone_return_pc = csr_read(0x8b2);  // DasicsFreeZoneReturnPC
    // Judge This is a dynamic call
    if (dasics_dynamic_call(regs)) return;

    uint64_t retval = 0;

    va_list args;
    UmaincallTypes type = (UmaincallTypes)regs->a0;

    regs->vararg_registers[0] = regs->a2;
    regs->vararg_registers[1] = regs->a3;
    regs->vararg_registers[2] = regs->a4;
    regs->vararg_registers[3] = regs->a5;
    regs->vararg_registers[4] = regs->a6;
    regs->vararg_registers[5] = regs->a7;
    args = (void *)regs->vararg_registers;

    switch (type)
    {
        case Umaincall_PRINT: {
            const char *format = (const char *)regs->a1;
#endif
            retval = vprintf(format, args);
        }
        break;

        default:
            printf("\x1b[33m Warning: Invalid umaincall number %d!\n\x1b[0m", type); //could not use printf in kernel
            break;
    }

#ifdef DASICS_NGINX_MODE1
    // csr_write(0x8b1, dasics_return_pc);             // DasicsReturnPC
    // csr_write(0x8b2, dasics_free_zone_return_pc);   // DasicsFreeZoneReturnPC

    va_end(args);
    return retval;
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
    int step = 4;

    // Iterate bounds table to find matching bounds
    hashed_bound_t *current, *temp;
    HASH_ITER(hh, bounds_table, current, temp) {
        if (current->bound.lo <= utval && utval < current->bound.hi && \
            (current->priv & valid_perm) == valid_perm && current->active) {
            // Find the matching bound, thus replace one libcfg & libbound with it
            // int victim = dasics_oldest_victim();
            int victim = rand() % DASICS_LIBCFG_WIDTH;
            LIBBOUND_LOOKUP(current->bound.hi, current->bound.lo, victim, WRITE);

            // Write config
            libcfg &= ~(DASICS_LIBCFG_MASK << (victim * step));
            libcfg |= ((uint64_t)current->priv) << (victim * step);
            csr_write(0x880, libcfg);   // DasicsLibCfg

            current->active = 1;
             // Fill dlibcsr map with new handle
            dlibcfg_handle_map[victim] = current->handle;
            current->map_idx = victim;
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

    uint64_t dasics_dfreason = csr_read(0x8b3);             // DasicsDFreason


    switch (dasics_dfreason)
    {
    case DFR_JUMP_DASICS_FAULT:
        error = udasics_fetch_fault_handler(regs);
        break;

    case DFR_LOAD_DASICS_FAULT:
        csr_idx = dasics_ldst_checker(regs->utval, 1);

        if (0 <= csr_idx && csr_idx < DASICS_LIBCFG_WIDTH) {
            uint64_t hi, lo;
            LIBBOUND_LOOKUP(hi, lo, csr_idx, READ);
            dasics_printf("[DASICS EXCEPTION]Info: dasics uload fault OK! new csr idx is %d, lo = %#lx, hi = %#lx\n", \
                csr_idx, lo, hi);
            return;
        }

        error = udasics_load_fault_handler(regs);
        break;

    case DFR_STORE_DASICS_FAULT:
        csr_idx = dasics_ldst_checker(regs->utval, 0);

        if (0 <= csr_idx && csr_idx < DASICS_LIBCFG_WIDTH) {
            uint64_t hi, lo;
            LIBBOUND_LOOKUP(hi, lo, csr_idx, READ);
            dasics_printf("[DASICS EXCEPTION]Info: dasics ustore fault OK! new csr idx is %d, lo = %#lx, hi = %#lx\n", \
                csr_idx, lo, hi);
            return;
        }

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
    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
    int32_t step = 4;

    lo = align8down(lo);
    hi = align8up(hi);

    // Insert new bound information to hash table
    hashed_bound_t *entry = (hashed_bound_t *)malloc(sizeof(hashed_bound_t));
    entry->bound.hi = hi;
    entry->bound.lo = lo;
    entry->active = 1;
    entry->priv = (cfg & DASICS_LIBCFG_MASK) | DASICS_LIBCFG_V;
    entry->handle = available_handle++;
#else
    regs->t1 = regs->ra;
    regs->a0 = retval;
    // csr_write(0x8b1, dasics_return_pc);             // DasicsReturnPC
    // csr_write(0x8b2, dasics_free_zone_return_pc);   // DasicsFreeZoneReturnPC


}

/* Refill only a complete access authorized by one software bound. Matching
 * utval alone would incorrectly retry a split access or weaken AMO/CBO rules. */
static int dasics_fault_access(const struct ucontext_trap *regs,
                               uint64_t *lo, uint64_t *hi,
                               unsigned int *permission, int *any_permission)
{
    const volatile unsigned char *pc = (const void *)regs->uepc;
    uint32_t instruction = pc[0] | ((uint32_t)pc[1] << 8);
    unsigned int size = 0, opcode, function;
    unsigned long expected_reason = 0;
    *lo = regs->utval;
    *any_permission = 0;
    if ((instruction & 3) != 3) {
        unsigned int quadrant = instruction & 3;
        function = instruction >> 13;
        if (quadrant != 0 && quadrant != 2) return -1;
        if (function == 1 || function == 2 || function == 3) {
            size = function == 2 ? 4 : 8;
            *permission = DASICS_LIBCFG_R;
            expected_reason = DFR_LOAD_DASICS_FAULT;
        } else if (function == 5 || function == 6 || function == 7) {
            size = function == 6 ? 4 : 8;
            *permission = DASICS_LIBCFG_W;
            expected_reason = DFR_STORE_DASICS_FAULT;
        } else return -1;
    } else {
        instruction |= ((uint32_t)pc[2] << 16) | ((uint32_t)pc[3] << 24);
        opcode = instruction & 0x7f;
        function = (instruction >> 12) & 7;
        if (opcode == 0x03 && function <= 6) {
            size = 1U << (function & 3);
            *permission = DASICS_LIBCFG_R;
            expected_reason = DFR_LOAD_DASICS_FAULT;
        } else if (opcode == 0x23 && function <= 3) {
            size = 1U << function;
            *permission = DASICS_LIBCFG_W;
            expected_reason = DFR_STORE_DASICS_FAULT;
        } else if ((opcode == 0x07 || opcode == 0x27) &&
                   (function == 2 || function == 3)) {
            size = 1U << function;
            *permission = opcode == 0x07 ? DASICS_LIBCFG_R : DASICS_LIBCFG_W;
            expected_reason = opcode == 0x07 ? DFR_LOAD_DASICS_FAULT : DFR_STORE_DASICS_FAULT;
        } else if (opcode == 0x2f && (function == 2 || function == 3)) {
            unsigned int operation = instruction >> 27;
            if (operation != 0 && operation != 1 && operation != 2 && operation != 3 &&
                operation != 4 && operation != 8 && operation != 12 && operation != 16 &&
                operation != 20 && operation != 24 && operation != 28) return -1;
            if (operation == 2 && ((instruction >> 20) & 31)) return -1;
            size = 1U << function;
            *permission = operation == 2 ? DASICS_LIBCFG_R :
                operation == 3 ? DASICS_LIBCFG_W : DASICS_LIBCFG_R | DASICS_LIBCFG_W;
            expected_reason = operation == 2 ? DFR_LOAD_DASICS_FAULT : DFR_STORE_DASICS_FAULT;
        } else if (opcode == 0x0f && function == 2 &&
                   (instruction >> 20) <= 4 && (instruction >> 20) != 3) {
            unsigned int operation = instruction >> 20;
            size = DASICS_CACHE_BLOCK_BYTES;
            *lo &= ~(uint64_t)(DASICS_CACHE_BLOCK_BYTES - 1);
            *any_permission = operation == 1 || operation == 2;
            *permission = *any_permission ? DASICS_LIBCFG_R | DASICS_LIBCFG_W : DASICS_LIBCFG_W;
            expected_reason = DFR_STORE_DASICS_FAULT;
        } else return -1;
    }
    if (regs->dfreason != expected_reason || *lo >= DASICS_USER_LIMIT ||
        size > DASICS_USER_LIMIT - *lo) return -1;
    *hi = *lo + size;
    return 0;
}

static int dasics_ldst_checker(const struct ucontext_trap *regs)
{
    uint64_t lo, hi, libcfg = csr_read(CSR_DLCFG);
    unsigned int permission;
    int any_permission;
    hashed_bound_t *current, *temp;
    if (dasics_fault_access(regs, &lo, &hi, &permission, &any_permission)) return -1;
    HASH_ITER(hh, bounds_table, current, temp) {
        int resident = 0;
        unsigned int present = current->priv & permission;
        if (!(current->priv & DASICS_LIBCFG_V) ||
            (any_permission ? !present : present != permission) ||
            lo < current->bound.lo || hi > current->bound.hi) continue;
        for (int i = 0; i < DASICS_LIBCFG_WIDTH; ++i)
            if (dlibcfg_handle_map[i] == current->handle) resident = 1;
        /* A fault with its complete authorization already resident is not a
         * cache miss. Do not retry it forever or replace a different grant. */
        if (resident) return -1;
        int victim = 0;
        while (victim < DASICS_LIBCFG_WIDTH &&
               ((libcfg >> (victim * 4)) & DASICS_LIBCFG_V)) ++victim;
        if (victim == DASICS_LIBCFG_WIDTH) victim = rand() % DASICS_LIBCFG_WIDTH;
        csr_write(CSR_DLCFG, libcfg & ~(DASICS_LIBCFG_V << (victim * 4)));
        LIBBOUND_LOOKUP(current->bound.hi, current->bound.lo, victim, WRITE);
        libcfg &= ~(DASICS_LIBCFG_MASK << (victim * 4));
        libcfg |= (uint64_t)current->priv << (victim * 4);
        csr_write(CSR_DLCFG, libcfg);
        dlibcfg_handle_map[victim] = current->handle;
        return victim;
    }
    return -1;
}

void dasics_ufault_handler(struct ucontext_trap *regs)
{
    int error = -1;
    regs->syscall_action = 0;
    if (regs->ucause != EXC_DASICS_UCHECK_FAULT) goto denied;
    switch (regs->dfreason) {
    case DFR_JUMP_DASICS_FAULT:
        error = udasics_fetch_fault_handler(regs);
        break;
    case DFR_LOAD_DASICS_FAULT:
        if (dasics_ldst_checker(regs) >= 0) return;
        error = udasics_load_fault_handler(regs);
        break;
    case DFR_STORE_DASICS_FAULT:
        if (dasics_ldst_checker(regs) >= 0) return;
        error = udasics_store_fault_handler(regs);
        break;
    case DFR_ECALL_DASICS_FAULT:
        error = udasics_ecall_fault_handler(regs);
        break;
    default:
        break;
    }
    if (error >= 0 && (regs->syscall_action == 0 ||
        (regs->dfreason == DFR_ECALL_DASICS_FAULT &&
         regs->syscall_action == DASICS_TRAP_PROXY_SYSCALL))) return;
denied:
    dasics_printf("[DASICS_EXCEPTION] denied cause=%lu reason=%lu pc=0x%lx address=0x%lx\n",
                  regs->ucause, regs->dfreason, regs->uepc, regs->utval);
    exit(1);
}

int32_t dasics_libcfg_alloc(uint64_t cfg, uint64_t lo, uint64_t hi) {
    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
    int32_t step = 4;

    if (dasics_normalize_bound(&lo, &hi) || available_handle == INT_MAX) return -1;

    // Insert new bound information to hash table
    hashed_bound_t *entry = (hashed_bound_t *)malloc(sizeof(hashed_bound_t));
    if (!entry) return -1;
    entry->bound.hi = hi;
    entry->bound.lo = lo;
    entry->priv = (cfg & DASICS_LIBCFG_MASK) | DASICS_LIBCFG_V;
    entry->handle = available_handle++;
    HASH_ADD_INT(bounds_table, handle, entry);

#endif
    // Find a proper libcfg for the newly allocated bound
    int32_t victim = 0;
    for (; victim < max_cfgs; ++victim) {
        uint64_t curr_cfg = (libcfg >> (victim * step)) & DASICS_LIBCFG_MASK;

        // Found available config
        if ((curr_cfg & DASICS_LIBCFG_V) == 0) {
#ifdef DASICS_NGINX_MODE1
            if (dlibcfg_handle_map[victim] != -1)
                continue;
            else
                break;
        }
    }

    if (victim == max_cfgs) {
        victim = 0;
        for (; victim < max_cfgs; ++victim) {
            uint64_t curr_cfg = (libcfg >> (victim * step)) & DASICS_LIBCFG_MASK;

            // Found available config
            if ((curr_cfg & DASICS_LIBCFG_V) == 0) {
                hashed_bound_t *old_entry;
                HASH_FIND_INT(bounds_table, &dlibcfg_handle_map[victim], old_entry);

                if (NULL != old_entry) {
                    old_entry->active = 0;
                    old_entry->map_idx = -1;
                }

                break;
            }
        }
    }


    // Kick out the oldest victim if we cannot find one available place
    if (victim == max_cfgs) {
        // victim = dasics_oldest_victim();
        victim = rand() % DASICS_LIBCFG_WIDTH;
        hashed_bound_t *old_entry;
        HASH_FIND_INT(bounds_table, &dlibcfg_handle_map[victim], old_entry);

        if (NULL != old_entry) {
            old_entry->active = 0;
            old_entry->map_idx = -1;
        }
    }

    // Write libbound
#else
            break;
        }
    }

    // Replace a resident entry when every hardware slot is in use.
    if (victim == max_cfgs) {
        victim = rand() % DASICS_LIBCFG_WIDTH;
    }

    // Disable the replaced slot while its two endpoints are updated.
    csr_write(CSR_DLCFG, libcfg & ~(DASICS_LIBCFG_V << (victim * step)));
#endif
    LIBBOUND_LOOKUP(hi, lo, victim, WRITE);

    // Write config
    libcfg &= ~(DASICS_LIBCFG_MASK << (victim * step));
    libcfg |= ((uint64_t)entry->priv) << (victim * step);
    csr_write(0x880, libcfg);   // DasicsLibCfg

#ifdef DASICS_NGINX_MODE1
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
    entry->map_idx = victim;
    HASH_ADD_INT(bounds_table, handle, entry);
#else
    // Fill dlibcsr map with new handle
    dlibcfg_handle_map[victim] = entry->handle;
#endif

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
#ifndef DASICS_NGINX_MODE1
            uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
#endif
            libcfg &= ~(DASICS_LIBCFG_V << (idx * step));
            csr_write(0x880, libcfg);   // DasicsLibCfg
            dlibcfg_handle_map[idx] = -1;
            break;
        }
    }

    return 0;
}

#ifdef DASICS_NGINX_MODE1
int32_t dasics_libcfg_free_all()
{
    int32_t idx = 0;

    for (; idx < DASICS_LIBCFG_WIDTH; ++idx) {
        if (dlibcfg_handle_map[idx] != -1) {
            dasics_libcfg_free(dlibcfg_handle_map[idx]);
        }
    }
#else
int32_t dasics_libcfg_free_all(void)
{
    hashed_bound_t *current, *temp;
    HASH_ITER(hh, bounds_table, current, temp) {
        HASH_DEL(bounds_table, current);
        free(current);
    }
    csr_write(CSR_DLCFG, 0);
    for (int i = 0; i < DASICS_LIBCFG_WIDTH; ++i) dlibcfg_handle_map[i] = -1;
#endif
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

#ifdef DASICS_NGINX_MODE1
int32_t dasics_libcfg_active(int32_t* handle, int num)
{
    for (int i = 0; i < num; i++)
    {
        if (handle[i] < 0) {
            return -1;
        }

        // Lookup hashed table firstly
        hashed_bound_t *entry;
        HASH_FIND_INT(bounds_table, &handle[i], entry);

        if (NULL == entry) {
            return -1;
        }

        entry->active = 1;

        if (entry->map_idx == -1)
        {
            // Find a proper libcfg for the newly allocated bound
            int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
            int32_t step = 4;
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
                hashed_bound_t *old_entry;
                HASH_FIND_INT(bounds_table, &dlibcfg_handle_map[victim], old_entry);

                if (NULL != old_entry) {
                    old_entry->active = 0;
                    old_entry->map_idx = -1;
                }
            }

            // Write libbound
            LIBBOUND_LOOKUP(entry->bound.hi, entry->bound.lo, victim, WRITE);

            // Write config
            libcfg &= ~(DASICS_LIBCFG_MASK << (victim * step));
            libcfg |= ((uint64_t)entry->priv) << (victim * step);

            // Fill dlibcsr map with new handle
            dlibcfg_handle_map[victim] = entry->handle;
            entry->map_idx = victim;
        } else
        {
            uint64_t step = 4;
            libcfg &= ~(DASICS_LIBCFG_MASK << (entry->map_idx * step));
            libcfg |= ((uint64_t)entry->priv) << (entry->map_idx * step);
        }
    }
    csr_write(0x880, libcfg);   // DasicsLibCfg

    return 0;
}


int32_t dasics_libcfg_inactive(int32_t* handle, int num)
{
    for (int i = 0; i < num; i++)
    {
        if (handle[i] < 0) {
            return -1;
        }
        // Lookup hashed table firstly
        hashed_bound_t *entry;
        HASH_FIND_INT(bounds_table, &handle[i], entry);

        if (NULL == entry) {
            return -1;
        }

        entry->active = 0;

        if (entry->map_idx != -1)
        {
            // Write config
            libcfg &= ~(DASICS_LIBCFG_V << (entry->map_idx * 4));
        }
    }
    // update the libcfg
    csr_write(0x880, libcfg);   // DasicsLibCfg
    return 0;
}


int32_t dasics_jumpcfg_alloc(uint64_t lo, uint64_t hi)
{
#else
int32_t dasics_jumpcfg_alloc(uint64_t lo, uint64_t hi)
{
    if (dasics_normalize_bound(&lo, &hi)) return -1;
    uint64_t jumpcfg = csr_read(0x8c8);    // DasicsJumpCfg
#endif
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
#ifndef DASICS_NGINX_MODE1
    uint64_t jumpcfg = csr_read(0x8c8);    // DasicsJumpCfg
#endif
    jumpcfg &= ~(DASICS_JUMPCFG_V << (idx * step));
    csr_write(0x8c8, jumpcfg); // DasicsJumpCfg
    return 0;
}


void dasics_print_cfg_register(int32_t handle)
{
	printf("DASICS uLib CFG Registers: handle:%x  config: %x \n",handle,dasics_libcfg_get(handle));
}


#ifndef DASICS_NGINX_MODE1

_Static_assert(sizeof(struct ucontext_trap) == OFFSET_SIZE, "trap frame size");
_Static_assert(offsetof(struct ucontext_trap, gp) == OFFSET_TRAP_GP, "trap prefix");
_Static_assert(offsetof(struct ucontext_trap, fpr) == OFFSET_TRAP_FPR, "trap FP offset");
_Static_assert(offsetof(struct ucontext_trap, syscall_action) == OFFSET_TRAP_SYSCALL_ACTION, "trap action offset");
_Static_assert(sizeof(struct umaincall) == OFFSET_UMAINCALL, "maincall frame size");
_Static_assert(offsetof(struct umaincall, vararg_registers) == OFFSET_UMAINCALL_VARARGS, "maincall argument area");
#endif
