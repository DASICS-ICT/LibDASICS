#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
// #include <machine/syscall.h>
#include "udasics.h"
#include <utrap.h>
#include <dasics_stdio.h>
#include <umaincall.h>

uint64_t umaincall_helper;

utrap_handler udasics_ecall_fault_handler = handle_DasicsUEcallFault;
utrap_handler udasics_load_fault_handler  = handle_DasicsULoadFault;
utrap_handler udasics_store_fault_handler = handle_DasicsUStoreFault;
utrap_handler udasics_fetch_fault_handler = handle_DasicsUFetchFault;

uint64_t libcfg = 0;
uint64_t jumpcfg = 0;

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

#define JMPBOUND_LOOKUP(HI,LO,IDX,OP) \
        switch (IDX + 0x18) \
        {               \
            CONCAT(OP)(HI,LO,0x18);  \
            CONCAT(OP)(HI,LO,0x19);  \
            CONCAT(OP)(HI,LO,0x1a);  \
            CONCAT(OP)(HI,LO,0x1b);  \
            default: \
                printf("\x1b[31m%s\x1b[0m","[DASICS]Error: out of jmpbound register range\n"); \
                break; \
        }

typedef struct {
    uint64_t lo;
    uint64_t hi;
} bound_t;

void register_udasics(uint64_t funcptr) 
{
    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
    int32_t step = 4;
    // Set random seed
    srand(2023);

    // Set maincall & ufault handler  
    umaincall_helper = (funcptr != 0) ? funcptr : (uint64_t) dasics_umaincall_helper;
    csr_write(0x8b0, (uint64_t)dasics_umaincall);
    csr_write(0x005, (uint64_t)dasics_ufault_entry);
}

void unregister_udasics(void) 
{
    // csr_write(0x8b0, 0);
    // csr_write(0x005, 0);
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
        case Umaincall_NULL: {
            // Do nothing, just return
            retval = 0;
            break;
        }
        case Umaincall_PRINT: {
            const char *format = va_arg(args, const char *);
            retval = vprintf(format, args);
        }
        break;

        default:
            printf("\x1b[33m Warning: Invalid umaincall number %d!\n\x1b[0m", type); //could not use printf in kernel
            break;
    }

    // csr_write(0x8b1, dasics_return_pc);             // DasicsReturnPC
    // csr_write(0x8b2, dasics_free_zone_return_pc);   // DasicsFreeZoneReturnPC

    va_end(args);
    return retval;
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
        error = udasics_load_fault_handler(regs);
        break;

    case EXC_DASICS_USTORE_FAULT:
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
    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
    int32_t step = 4;

    lo = align8down(lo);
    hi = align8up(hi);

    // Find a proper libcfg for the newly allocated bound
    int32_t victim = 0;
    for (; victim < max_cfgs; ++victim) {
        uint64_t curr_cfg = (libcfg >> (victim * step)) & DASICS_LIBCFG_MASK;

        // Found available config
        if ((curr_cfg & DASICS_LIBCFG_V) == 0) {
            break;
        }
    }

    if (victim == max_cfgs) {
        return -1;
    }

    // Write libbound
    LIBBOUND_LOOKUP(hi, lo, victim, WRITE);

    // Write config
    libcfg &= ~(DASICS_LIBCFG_MASK << (victim * step));
    libcfg |= ((uint64_t)((cfg & DASICS_LIBCFG_MASK) | DASICS_LIBCFG_V)) << (victim * step);
    csr_write(0x880, libcfg);   // DasicsLibCfg

    return victim;
}

int32_t dasics_libcfg_free(int32_t handle) {
    if (handle < 0 || handle >= DASICS_LIBCFG_WIDTH) {
        return -1;
    }

    // Check if the target bound exists in dasics CSRs
    int32_t step = 4;
    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
    libcfg &= ~(DASICS_LIBCFG_V << (handle * step));
    csr_write(0x880, libcfg);   // DasicsLibCfg

    return 0;
}

int32_t dasics_libcfg_free_all()
{
    csr_write(0x880, 0); // Clear DasicsLibCfg
    return 0;
}

uint32_t dasics_libcfg_get(int32_t handle) {
    if (handle < 0 || handle >= DASICS_LIBCFG_WIDTH) {
        return -1; // Invalid handle
    }

    int32_t step = 4;
    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
    uint64_t cfg = (libcfg >> (handle * step)) & DASICS_LIBCFG_MASK;

    return cfg;
}

int32_t dasics_libcfg_active(int32_t* handle, int num)
{
    for (int i = 0; i < num; i++)
    {
        if (handle[i] < 0 || handle[i] >= DASICS_LIBCFG_WIDTH) {
            return -1;
        }

        uint64_t step = 4;
        libcfg |= (DASICS_LIBCFG_V << (handle[i] * step));
    }
    csr_write(0x880, libcfg);   // DasicsLibCfg

    return 0;
}


int32_t dasics_libcfg_inactive(int32_t* handle, int num)
{
    for (int i = 0; i < num; i++)
    {
        if (handle[i] < 0 || handle[i] >= DASICS_LIBCFG_WIDTH) {
            return -1;
        }

        // Write config
        uint64_t step = 4;
        libcfg &= ~(DASICS_LIBCFG_V << (handle[i] * step));
    }
    // update the libcfg
    csr_write(0x880, libcfg);   // DasicsLibCfg
    return 0;
}


int32_t dasics_jumpcfg_alloc(uint64_t lo, uint64_t hi)
{
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
    jumpcfg &= ~(DASICS_JUMPCFG_V << (idx * step));
    csr_write(0x8c8, jumpcfg); // DasicsJumpCfg
    return 0;
}


void dasics_print_cfg_register(int32_t handle)
{
	printf("DASICS uLib CFG Registers: handle:%x  config: %x \n",handle,dasics_libcfg_get(handle));
}

int32_t ATTR_ULIB_CALLER_TEXT dasics_ulib_libcfg_alloc(uint64_t cfg, uint64_t lo, uint64_t hi) {
    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
    uint64_t mem_bound_status = dasics_ulib_query_bound(TYPE_MEM_BOUND);
    int32_t target_idx,orig_idx;
    for (target_idx= 0; target_idx < max_cfgs; ++target_idx) {
        uint64_t curr_status = (mem_bound_status >> (target_idx * 2)) & BNDQUERY_MASK;

        if (curr_status == BNDQUERY_EMPTY) {
            // try to find origin libcfg
            for (orig_idx = 0; orig_idx < max_cfgs; ++orig_idx){
                uint64_t orig_status = (mem_bound_status >> (orig_idx * 2)) & BNDQUERY_MASK;
                if (orig_status == 0x1){ // libcfg in the same level
                    uint64_t orig_cfg = (libcfg >> (orig_idx * 4)) & DASICS_LIBCFG_MASK;
                    uint64_t orig_lo,orig_hi;
                    LIBBOUND_LOOKUP(orig_hi, orig_lo, orig_idx, READ); // read origin libcfg
                    if (orig_lo <= lo && hi <= orig_hi && !(cfg & ~orig_cfg)) break; // current field smaller than origin, OK
                }
            }
            if (orig_idx == max_cfgs) return -1; // no origin libcfg
            dasics_ulib_copy_bound(TYPE_MEM_BOUND,orig_idx,target_idx); // copy origin libcfg to target
            LIBBOUND_LOOKUP(hi, lo, target_idx, WRITE); // modify target libcfg according to arg

            // Write config
            libcfg &= ~(DASICS_LIBCFG_MASK << (target_idx * 4));
            libcfg |= (uint64_t)((cfg & DASICS_LIBCFG_MASK) | DASICS_LIBCFG_V) << (target_idx * 4);
            csr_write(0x880, libcfg);   // DasicsLibCfg

            return target_idx;
        }
    }
    return -1;
}

int32_t ATTR_ULIB_CALLER_TEXT dasics_ulib_libcfg_copy(int src_idx) {
    if (src_idx < 0 || src_idx >= DASICS_LIBCFG_WIDTH) return -1;

    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
    uint64_t mem_bound_status = dasics_ulib_query_bound(TYPE_MEM_BOUND);
    uint64_t src_status = (mem_bound_status >> (src_idx * 2)) & BNDQUERY_MASK;
    if (src_status == BNDQUERY_DENY || src_status == BNDQUERY_EMPTY) return -1;  // cannot copy
    int32_t target_idx;
    for (target_idx= 0; target_idx < max_cfgs; ++target_idx) {
        uint64_t curr_status = (mem_bound_status >> (target_idx * 2)) & BNDQUERY_MASK;

        if (curr_status == BNDQUERY_EMPTY){
            dasics_ulib_copy_bound(TYPE_MEM_BOUND,src_idx,target_idx); // copy origin libcfg to target
            return target_idx;
        }
    }
    return -1;
}

int32_t ATTR_ULIB_CALLER_TEXT dasics_ulib_libcfg_free(int32_t idx) {
    if (idx < 0 || idx >= DASICS_LIBCFG_WIDTH) return -1;

    uint64_t mem_bound_status = dasics_ulib_query_bound(TYPE_MEM_BOUND);
    uint64_t tiny_status = (mem_bound_status >> (idx * 2)) & BNDQUERY_MASK;
    if (tiny_status != BNDQUERY_RW) return -1; // no permission

    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
    libcfg &= ~(DASICS_LIBCFG_V << (idx * 4));
    csr_write(0x880, libcfg);   // DasicsLibCfg
    return 0;
}

int32_t ATTR_ULIB_CALLER_TEXT dasics_ulib_mem_get(int32_t idx, uint64_t *lo, uint64_t *hi) {
    if (idx < 0 || idx >= DASICS_LIBCFG_WIDTH) return -1;

    uint64_t mem_bound_status = dasics_ulib_query_bound(TYPE_MEM_BOUND);
    uint64_t tiny_status = (mem_bound_status >> (idx * 2)) & BNDQUERY_MASK;
    if (tiny_status != BNDQUERY_RO && tiny_status != BNDQUERY_RW) return -1; // not readable

    LIBBOUND_LOOKUP(*hi, *lo, idx, READ); // read libcfg
    return 0;
}

int32_t ATTR_ULIB_CALLER_TEXT dasics_ulib_jumpcfg_alloc(uint64_t lo, uint64_t hi) {
    uint64_t jumpcfg = csr_read(0x8c8);    // DasicsJumpCfg
    int32_t max_cfgs = DASICS_JUMPCFG_WIDTH;
    uint64_t jmp_bound_status = dasics_ulib_query_bound(TYPE_JMP_BOUND);
    int32_t target_idx,orig_idx;
    for (target_idx = 0; target_idx < max_cfgs; ++target_idx) {
        uint64_t curr_status = (jmp_bound_status >> (target_idx * 2)) & BNDQUERY_MASK;
        if (curr_status == BNDQUERY_EMPTY) // found available cfg
        {
            // try to find origin jmpcfg
            for (orig_idx = 0; orig_idx < max_cfgs; ++orig_idx){
                uint64_t orig_status = (jmp_bound_status >> (orig_idx * 2)) & BNDQUERY_MASK;
                if (orig_status == BNDQUERY_RO){ // jmpcfg in the same level
                    uint64_t orig_lo,orig_hi;
                    JMPBOUND_LOOKUP(orig_hi, orig_lo, orig_idx, READ); // read origin jmpcfg
                    if (orig_lo <= lo && hi <= orig_hi) break; // current field smaller than origin, OK
                }
            }
            if (orig_idx == max_cfgs) return -1; // no origin jmpcfg
            dasics_ulib_copy_bound(TYPE_JMP_BOUND,orig_idx,31); // copy origin jmpcfg to scratchpad
            csr_write(0x8d2, lo);  // scratchpad_lo
            csr_write(0x8d3, hi);  // scratchpad_hi
            dasics_ulib_copy_bound(TYPE_JMP_BOUND,31,target_idx); // copy scratchpad to target jmpcfg

            jumpcfg &= ~(DASICS_JUMPCFG_MASK << (target_idx * 16));
            jumpcfg |= DASICS_JUMPCFG_V << (target_idx * 16);
            csr_write(0x8c8, jumpcfg); // DasicsJumpCfg
            return target_idx;
        }
    }
    return -1;
}

int32_t ATTR_ULIB_CALLER_TEXT dasics_ulib_jumpcfg_free(int32_t idx) {
    if (idx < 0 || idx >= DASICS_JUMPCFG_WIDTH) return -1;

    uint64_t jmp_bound_status = dasics_ulib_query_bound(TYPE_JMP_BOUND);
    uint64_t tiny_status = (jmp_bound_status >> (idx * 2)) & BNDQUERY_MASK;
    if (tiny_status != BNDQUERY_RW) return -1; // no permission

    uint64_t jumpcfg = csr_read(0x8c8);    // DasicsJumpCfg
    jumpcfg &= ~(DASICS_JUMPCFG_V << (idx * 16));
    csr_write(0x8c8, jumpcfg); // DasicsJumpCfg
    return 0;
}

int32_t ATTR_ULIB_CALLER_TEXT dasics_ulib_jump_get(int32_t idx, uint64_t *lo, uint64_t *hi) {
    if (idx < 0 || idx >= DASICS_JUMPCFG_WIDTH) return -1;

    uint64_t jmp_bound_status = dasics_ulib_query_bound(TYPE_JMP_BOUND);
    uint64_t tiny_status = (jmp_bound_status >> (idx * 2)) & BNDQUERY_MASK;
    if (tiny_status != BNDQUERY_RO && tiny_status != BNDQUERY_RW) return -1; // not readable

    JMPBOUND_LOOKUP(*hi, *lo, idx, READ); // read jmpcfg
    return 0;
}