#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
// #include <machine/syscall.h>
#include "udasics.h"
#include "uthash.h"
#include <utrap.h>
#include <dasics_stdio.h>
#include <umaincall.h>
#include <uwrapper.h>
#include <time.h>

uint64_t umaincall_helper;
permission_t obstack[4096];
uint64_t stack_top = 4095;


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

#define JMPBOUND_REG_READ(hi,lo,idx)   \
        case idx:  \
            lo = csr_read(0x8c0 + idx * 2);  \
            hi = csr_read(0x8c1 + idx * 2);  \
            break;

#define JMPBOUND_REG_WRITE(hi,lo,idx)   \
        case idx:  \
            csr_write(0x8c0 + idx * 2, lo);  \
            csr_write(0x8c1 + idx * 2, hi);  \
            break;
#define JMPCONCAT(OP) JMPBOUND_REG_##OP

#define JMPBOUND_LOOKUP(HI,LO,IDX,OP) \
        switch (IDX) \
        {               \
            JMPCONCAT(OP)(HI,LO,0);  \
            JMPCONCAT(OP)(HI,LO,1);  \
            JMPCONCAT(OP)(HI,LO,2);  \
            JMPCONCAT(OP)(HI,LO,3);  \
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
    // srand(2023);

    // Write OS-allocated bounds to hash table
    // for (int32_t idx = 0; idx < max_cfgs; ++idx) {
    //     uint64_t curr_cfg = (libcfg >> (idx * step)) & DASICS_LIBCFG_MASK;

    //     // Found allocated config
    //     if ((curr_cfg & DASICS_LIBCFG_V) != 0) {
    //         uint64_t hi, lo;
    //         LIBBOUND_LOOKUP(hi, lo, idx, READ);
    //         hashed_bound_t *entry = (hashed_bound_t *)malloc(sizeof(hashed_bound_t));
    //         entry->bound.hi = hi;
    //         entry->bound.lo = lo;
    //         entry->priv = curr_cfg;
    //         entry->handle = available_handle++;
    //         HASH_ADD_INT(bounds_table, handle, entry);
    //         dlibcfg_handle_map[idx] = entry->handle;
    //     }
    // }

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
    // hashed_bound_t *current, *temp;
    // HASH_ITER(hh, bounds_table, current, temp) {
    //     HASH_DEL(bounds_table, current);
    //     free(current);
    // }
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
        if (cfg == -1 || (cfg & DASICS_LIBCFG_V) == 0) {
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

static long invoke_syscall(long sysno, long arg0, long arg1, long arg2,
                           long arg3, long arg4, long arg5, long arg6)
{
    register long a7 asm("a7") = sysno;
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a2 asm("a2") = arg2;
    register long a3 asm("a3") = arg3;
    register long a4 asm("a4") = arg4;
    register long a5 asm("a5") = arg5;
    register long a6 asm("a6") = arg6;

    asm volatile("ecall"                      \
                 : "+r"(a0)                   \
                 : "r"(a1), "r"(a2), "r"(a3), \
                   "r"(a4), "r"(a5), "r"(a6), \
                   "r"(a7)                    \
                 : "memory");

    return a0;
}

uint64_t dasics_umaincall_helper(struct umaincall * regs, ...)
{
    // uint64_t dasics_return_pc = csr_read(0x8b1);            // DasicsReturnPC
    // uint64_t dasics_free_zone_return_pc = csr_read(0x8b2);  // DasicsFreeZoneReturnPC
    // Judge This is a dynamic call
    // if (dasics_dynamic_call(regs)) return 0;

    // uint64_t retval = 0;
    regs->t1 = regs->ra;
    if (regs->ra == (uint64_t)&dasics_umaincall)
    {
        // pop permission
        permission_t *restore = &obstack[++stack_top];
        csr_write(0x880, restore->dasicsLibCfg);
        csr_write(0x8c8, restore->dasicsJumpCfg);
        // Set new permission
        for (size_t i = 0; i < DASICS_LIBCFG_WIDTH; i++)
        {
            uint64_t hi, lo;
            lo = restore->dasicsLibBounds[i * 2];
            hi = restore->dasicsLibBounds[i * 2 + 1];

            LIBBOUND_LOOKUP(hi, lo, i, WRITE);                
        }
        
        for (size_t i = 0; i < DASICS_JUMPCFG_WIDTH; i++)
        {
            uint64_t hi, lo;
            lo = restore->dasicsJumpBounds[i * 2];
            hi = restore->dasicsJumpBounds[i * 2 + 1];

            JMPBOUND_LOOKUP(hi, lo, i, WRITE);                
        }

        regs->t1 = restore->ra;
        return 0;
    }

    va_list args;
    UmaincallTypes type = (UmaincallTypes)regs->a0;



    va_start(args, type);

    switch (type)
    {
        case Umaincall_PRINT: {
            const char *format = va_arg(args, const char *);
            vprintf(format, args);
        }
        break;

        case Umaincall_switch:{
            permission_t * permission = va_arg(args, permission_t *);
            void * target = va_arg(args, void *);
            reg_t hi, lo;
            // Check libbound
            int check_ok = 0;
            for (int i = 2; i < DASICS_LIBCFG_WIDTH; i++)
            {
                
                if (permission->dasicsLibCfg & (DASICS_LIBCFG_V << (i * 4)))
                {
                    uint64_t priv_s = (permission->dasicsLibCfg & (0xf << (i * 4))) >> (i * 4);
                    for (int j = 2; j < DASICS_LIBCFG_WIDTH; j++)
                    {
                        /* code */
                        LIBBOUND_LOOKUP(hi, lo, j, READ);

                        if (permission->dasicsLibBounds[i * 2] >= lo && permission->dasicsLibBounds[i * 2 + 1] <= hi)
                        {
                            uint64_t priv_n = (csr_read(0x880) & (0xful << (j * 4))) >> (j * 4);
                            uint64_t mask = priv_n & priv_s;
                            // printf("mask: 0x%lx, priv_n: 0x%lx\n", mask, priv_n);
                            if ((int64_t)mask <= (int64_t)priv_n)
                            {
                                check_ok = 1;
                                break;
                            }
                        }
                    }   
                    if (!check_ok) 
                    {
                        printf("Permisson deny\n");
                        return 0 ;
                    } else check_ok = 0;                    
                }

            }
            permission->dasicsLibCfg |= csr_read(0x880) & 0xfful;

            permission->dasicsLibBounds[0] = csr_read(0x890);
            permission->dasicsLibBounds[1] = csr_read(0x891);
            permission->dasicsLibBounds[2] = csr_read(0x892);
            permission->dasicsLibBounds[3] = csr_read(0x893);

            // Check jmp
            check_ok = 0;
            for (int i = 0; i < DASICS_JUMPCFG_WIDTH; i++)
            {
                if (permission->dasicsJumpCfg & (DASICS_JUMPCFG_V << (i * 16)))
                {
                    uint64_t priv_s = (permission->dasicsJumpCfg & (0xfful << (i * 16))) >> (i * 16);
                    for (int j = 0; j < DASICS_JUMPCFG_WIDTH; j++)
                    {
                        /* code */
                        JMPBOUND_LOOKUP(hi, lo, j, READ);


                        if (permission->dasicsJumpBounds[i * 2] >= lo && permission->dasicsJumpBounds[i * 2 + 1] <= hi)
                        {
                            uint64_t priv_n = (csr_read(0x8c8) & (0xfful << (j * 16))) >> (j * 16);
                            uint64_t mask = priv_n & priv_s;
                            // printf("mask: 0x%lx, priv_n: 0x%lx\n", mask, priv_n);
                            if ((int64_t)mask <= (int64_t)priv_n)
                            {
                                check_ok = 1;
                                break;
                            }
                        }
                    }   
                    if (!check_ok) 
                    {
                        printf("Permisson deny\n");
                        return 0 ;
                    } else check_ok = 0;
                }

            }

            uint64_t stack_now = stack_top--;
            obstack[stack_now].ra =  regs->ra;
            // obstack[stack_now].retpc =  csr_read(0x8b4);
            obstack[stack_now].dasicsLibCfg =  csr_read(0x880);
            obstack[stack_now].dasicsJumpCfg =  csr_read(0x8c8);
            // Save all permission on stack
            for (int i = 0; i < DASICS_LIBCFG_WIDTH; i++)
            {
                /* code */
                uint64_t hi, lo;
                LIBBOUND_LOOKUP(hi, lo, i, READ);
                obstack[stack_now].dasicsLibBounds[i * 2] = lo;
                obstack[stack_now].dasicsLibBounds[i * 2 + 1] = hi;
            }
            

            for (int i = 0; i < DASICS_JUMPCFG_WIDTH; i++)
            {
                /* code */
                uint64_t hi, lo;
                JMPBOUND_LOOKUP(hi, lo, i, READ);
                obstack[stack_now].dasicsJumpBounds[i * 2] = lo;
                obstack[stack_now].dasicsJumpBounds[i * 2 + 1] = hi;
            }    

            csr_write(0x880, permission->dasicsLibCfg);
            csr_write(0x8c8, permission->dasicsJumpCfg);
            // Set new permission
            for (int i = 0; i < DASICS_LIBCFG_WIDTH; i++)
            {
                uint64_t hi, lo;
                lo = permission->dasicsLibBounds[i * 2];
                hi = permission->dasicsLibBounds[i * 2 + 1];

                LIBBOUND_LOOKUP(hi, lo, i, WRITE);                
            }
            
            for (int i = 0; i < DASICS_JUMPCFG_WIDTH; i++)
            {
                uint64_t hi, lo;
                lo = permission->dasicsJumpBounds[i * 2];
                hi = permission->dasicsJumpBounds[i * 2 + 1];

                JMPBOUND_LOOKUP(hi, lo, i, WRITE);                
            }
            regs->ra = (uint64_t)&dasics_umaincall;
            regs->t1 = (uint64_t)target;

        }
        break;
        case Umaincall_GET_TICK:
            // Do sys_get_ticks
            regs->a0 = (uint64_t)invoke_syscall(248, 0 , 0, 0, 0, 0, 0, 0);
            break;

        case Umaincall_getclock:
            // Do sys_get_ticks
            regs->a0 = clock();     
            break;       
        default:
            printf("\x1b[33m%s\x1b[0m","Warning: Invalid umaincall number %d!\n", type); //could not use printf in kernel
            break;
    }

    
    // csr_write(0x8b1, dasics_return_pc);             // DasicsReturnPC
    // csr_write(0x8b2, dasics_free_zone_return_pc);   // DasicsFreeZoneReturnPC

    va_end(args);

    // return retval;
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
        error = handle_DasicsUFetchFault(regs);
        break;
    
    case EXC_DASICS_ULOAD_FAULT:
        csr_idx = dasics_ldst_checker(regs->utval, 1);

        if (0 <= csr_idx && csr_idx < DASICS_LIBCFG_WIDTH) {
            uint64_t hi, lo;
            LIBBOUND_LOOKUP(hi, lo, csr_idx, READ);
            printf("[DASICS EXCEPTION]Info: dasics uload fault OK! new csr idx is %d, lo = %#lx, hi = %#lx\n", \
                csr_idx, lo, hi);
            return;
        }

        error = handle_DasicsULoadFault(regs);
        break;

    case EXC_DASICS_USTORE_FAULT:
        csr_idx = dasics_ldst_checker(regs->utval, 0);

        if (0 <= csr_idx && csr_idx < DASICS_LIBCFG_WIDTH) {
            uint64_t hi, lo;
            LIBBOUND_LOOKUP(hi, lo, csr_idx, READ);
            printf("[DASICS EXCEPTION]Info: dasics ustore fault OK! new csr idx is %d, lo = %#lx, hi = %#lx\n", \
                csr_idx, lo, hi);
            return;
        }      

        error = handle_DasicsUStoreFault(regs);
        break;
    
    case EXC_DASICS_UECALL_FAULT:
        error = handle_DasicsUEcallFault(regs);
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
        return -1;
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


int32_t ATTR_ULIB1_TEXT dasics_ulib_libcfg_alloc(uint64_t cfg, uint64_t lo, uint64_t hi) {
    
    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
    uint64_t mem_bound_status = dasics_ulib_query_bound(TYPE_MEM_BOUND);
    int32_t target_idx,orig_idx;
    for (target_idx= 0; target_idx < max_cfgs; ++target_idx) {
        uint64_t curr_status = (mem_bound_status >> (target_idx * 2)) & 0x3;

        if (curr_status == 0x3){

            // try to find origin libcfg 
            for (orig_idx = 0; orig_idx < max_cfgs; ++orig_idx){
                uint64_t orig_status = (mem_bound_status >> (orig_idx * 2)) & 0x3;
                if (orig_status == 0x1){ // libcfg in the same level
                    uint64_t orig_cfg = (libcfg >> (orig_idx * 4)) & DASICS_LIBCFG_MASK;
                    uint64_t orig_lo,orig_hi;
                    switch (orig_idx) { // read origin libcfg
                    case 0:
                        orig_lo = csr_read(0x890);   // DasicsLibBound0Lo
                        orig_hi = csr_read(0x891);   // DasicsLibBound0Hi
                        break;
                    case 1:
                        orig_lo = csr_read(0x892);   // DasicsLibBound1Lo
                        orig_hi = csr_read(0x893);   // DasicsLibBound1Hi
                        break;
                    case 2:
                        orig_lo = csr_read(0x894);   // DasicsLibBound2Lo
                        orig_hi = csr_read(0x895);   // DasicsLibBound2Hi
                        break;
                    case 3:
                        orig_lo = csr_read(0x896);   // DasicsLibBound3Lo
                        orig_hi = csr_read(0x897);   // DasicsLibBound3Hi
                        break;
                    case 4:
                        orig_lo = csr_read(0x898);   // DasicsLibBound4Lo
                        orig_hi = csr_read(0x899);   // DasicsLibBound4Hi
                        break;
                    case 5:
                        orig_lo = csr_read(0x89a);   // DasicsLibBound5Lo
                        orig_hi = csr_read(0x89b);   // DasicsLibBound5Hi
                        break;
                    case 6:
                        orig_lo = csr_read(0x89c);   // DasicsLibBound6Lo
                        orig_hi = csr_read(0x89d);   // DasicsLibBound6Hi
                        break;
                    case 7:
                        orig_lo = csr_read(0x89e);   // DasicsLibBound7Lo
                        orig_hi = csr_read(0x89f);   // DasicsLibBound7Hi
                        break;
                    case 8:
                        orig_lo = csr_read(0x8a0);   // DasicsLibBound8Lo
                        orig_hi = csr_read(0x8a1);   // DasicsLibBound8Hi
                        break;
                    case 9:
                        orig_lo = csr_read(0x8a2);   // DasicsLibBound9Lo
                        orig_hi = csr_read(0x8a3);   // DasicsLibBound9Hi
                        break;
                    case 10:
                        orig_lo = csr_read(0x8a4);   // DasicsLibBound10Lo
                        orig_hi = csr_read(0x8a5);   // DasicsLibBound10Hi
                        break;
                    case 11:
                        orig_lo = csr_read(0x8a6);   // DasicsLibBound11Lo
                        orig_hi = csr_read(0x8a7);   // DasicsLibBound11Hi
                        break;
                    case 12:
                        orig_lo = csr_read(0x8a8);   // DasicsLibBound12Lo
                        orig_hi = csr_read(0x8a9);   // DasicsLibBound12Hi
                        break;
                    case 13:
                        orig_lo = csr_read(0x8aa);   // DasicsLibBound13Lo
                        orig_hi = csr_read(0x8ab);   // DasicsLibBound13Hi
                        break;
                    case 14:
                        orig_lo = csr_read(0x8ac);   // DasicsLibBound14Lo
                        orig_hi = csr_read(0x8ad);   // DasicsLibBound14Hi
                        break;
                    case 15:
                        orig_lo = csr_read(0x8ae);   // DasicsLibBound15Lo
                        orig_hi = csr_read(0x8af);   // DasicsLibBound15Hi
                        break;
                    default:
                        break;
                    }
                    if (orig_lo <= lo && hi <= orig_hi && !(cfg & ~orig_cfg)) break; // current field smaller than origin, OK
                }
            }
            if (orig_idx == max_cfgs) return -1; // no origin libcfg
            dasics_ulib_copy_bound(TYPE_MEM_BOUND,orig_idx,target_idx); // copy origin libcfg to target
            switch (target_idx) { // modify target libcfg according to arg
                case 0:
                    csr_write(0x890, lo);   // DasicsLibBound0Lo
                    csr_write(0x891, hi);   // DasicsLibBound0Hi
                    break;
                case 1:
                    csr_write(0x892, lo);   // DasicsLibBound1Lo
                    csr_write(0x893, hi);   // DasicsLibBound1Hi
                    break;
                case 2:
                    csr_write(0x894, lo);   // DasicsLibBound2Lo
                    csr_write(0x895, hi);   // DasicsLibBound2Hi
                    break;
                case 3:
                    csr_write(0x896, lo);   // DasicsLibBound3Lo
                    csr_write(0x897, hi);   // DasicsLibBound3Hi
                    break;
                case 4:
                    csr_write(0x898, lo);   // DasicsLibBound4Lo
                    csr_write(0x899, hi);   // DasicsLibBound4Hi
                    break;
                case 5:
                    csr_write(0x89a, lo);   // DasicsLibBound5Lo
                    csr_write(0x89b, hi);   // DasicsLibBound5Hi
                    break;
                case 6:
                    csr_write(0x89c, lo);   // DasicsLibBound6Lo
                    csr_write(0x89d, hi);   // DasicsLibBound6Hi
                    break;
                case 7:
                    csr_write(0x89e, lo);   // DasicsLibBound7Lo
                    csr_write(0x89f, hi);   // DasicsLibBound7Hi
                    break;
                case 8:
                    csr_write(0x8a0, lo);   // DasicsLibBound8Lo
                    csr_write(0x8a1, hi);   // DasicsLibBound8Hi
                    break;
                case 9:
                    csr_write(0x8a2, lo);   // DasicsLibBound9Lo
                    csr_write(0x8a3, hi);   // DasicsLibBound9Hi
                    break;
                case 10:
                    csr_write(0x8a4, lo);   // DasicsLibBound10Lo
                    csr_write(0x8a5, hi);   // DasicsLibBound10Hi
                    break;
                case 11:
                    csr_write(0x8a6, lo);   // DasicsLibBound11Lo
                    csr_write(0x8a7, hi);   // DasicsLibBound11Hi
                    break;
                case 12:
                    csr_write(0x8a8, lo);   // DasicsLibBound12Lo
                    csr_write(0x8a9, hi);   // DasicsLibBound12Hi
                    break;
                case 13:
                    csr_write(0x8aa, lo);   // DasicsLibBound13Lo
                    csr_write(0x8ab, hi);   // DasicsLibBound13Hi
                    break;
                case 14:
                    csr_write(0x8ac, lo);   // DasicsLibBound14Lo
                    csr_write(0x8ad, hi);   // DasicsLibBound14Hi
                    break;
                case 15:
                    csr_write(0x8ae, lo);   // DasicsLibBound15Lo
                    csr_write(0x8af, hi);   // DasicsLibBound15Hi
                    break;
                default:
                    break;
            }
            // Write config
            libcfg &= ~(DASICS_LIBCFG_MASK << (target_idx * 4));
            libcfg |= (cfg & DASICS_LIBCFG_MASK) << (target_idx * 4);
            csr_write(0x880, libcfg);   // DasicsLibCfg

            return target_idx;
        }
    }
    return -1;
}


int32_t ATTR_ULIB1_TEXT dasics_ulib_libcfg_copy(int src_idx) {
    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
    uint64_t mem_bound_status = dasics_ulib_query_bound(TYPE_MEM_BOUND);
    uint64_t src_status = (mem_bound_status >> (src_idx * 2)) & 0x3;
    if (src_status == 0 || src_status == 3) return -1;  // cannot copy
    int32_t target_idx;
    for (target_idx= 0; target_idx < max_cfgs; ++target_idx) {
        uint64_t curr_status = (mem_bound_status >> (target_idx * 2)) & 0x3;

        if (curr_status == 0x3){
            dasics_ulib_copy_bound(TYPE_MEM_BOUND,src_idx,target_idx); // copy origin libcfg to target
            return target_idx;
        }
    }
    return -1;
}

int32_t ATTR_ULIB1_TEXT dasics_ulib_libcfg_free(int32_t idx) {
    uint64_t mem_bound_status = dasics_ulib_query_bound(TYPE_MEM_BOUND);
    if (!(mem_bound_status >> (idx * 2) & 0x3)) return -1; // no permission

    if (idx < 0 || idx >= DASICS_LIBCFG_WIDTH) return -1;
    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
    libcfg &= ~(DASICS_LIBCFG_V << (idx * 4));
    csr_write(0x880, libcfg);   // DasicsLibCfg
    return 0;
}

// uint32_t dasics_libcfg_get(int32_t idx) {
//     if (idx < 0 || idx >= DASICS_LIBCFG_WIDTH) return -1;
//     uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
//     return (libcfg >> (idx * 4)) & DASICS_LIBCFG_MASK;
// }

int32_t ATTR_ULIB1_TEXT dasics_ulib_jumpcfg_alloc(uint64_t lo, uint64_t hi)
{
    uint64_t jumpcfg = csr_read(0x8c8);    // DasicsJumpCfg
    int32_t max_cfgs = DASICS_JUMPCFG_WIDTH;
    uint64_t jmp_bound_status = dasics_ulib_query_bound(TYPE_JMP_BOUND);
    int32_t target_idx,orig_idx;
    for (target_idx = 0; target_idx < max_cfgs; ++target_idx) {
        uint64_t curr_status = (jmp_bound_status >> (target_idx * 2)) & 0x3;
        if (curr_status == 0x3) // found available cfg
        {
            // try to find origin jmpcfg 
            for (orig_idx = 0; orig_idx < max_cfgs; ++orig_idx){
                uint64_t orig_status = (jmp_bound_status >> (orig_idx * 2)) & 0x3;
                if (orig_status == 0x1){ // jmpcfg in the same level
                    uint64_t orig_lo,orig_hi;
                    switch (orig_idx) { // read origin jmpcfg
                        case 0:
                            orig_lo = csr_read(0x8c0);  // DasicsJumpBound0Lo
                            orig_hi = csr_read(0x8c1);  // DasicsJumpBound0Hi
                            break;
                        case 1:
                            orig_lo = csr_read(0x8c2);  // DasicsJumpBound1Lo
                            orig_hi = csr_read(0x8c3);  // DasicsJumpBound1Hi
                            break;
                        case 2:
                            orig_lo = csr_read(0x8c4);  // DasicsJumpBound2Lo
                            orig_hi = csr_read(0x8c5);  // DasicsJumpBound2Hi
                            break;
                        case 3:
                            orig_lo = csr_read(0x8c6);  // DasicsJumpBound3Lo
                            orig_hi = csr_read(0x8c7);  // DasicsJumpBound3Hi
                            break;
                        default:
                            break;
                    }
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

int32_t ATTR_ULIB1_TEXT dasics_ulib_jumpcfg_free(int32_t idx) {
    uint64_t jmp_bound_status = dasics_ulib_query_bound(TYPE_JMP_BOUND);
    if (!(jmp_bound_status >> (idx * 2) & 0x3)) return -1; // no permission
    
    if (idx < 0 || idx >= DASICS_JUMPCFG_WIDTH) return -1;
    uint64_t jumpcfg = csr_read(0x8c8);    // DasicsJumpCfg
    jumpcfg &= ~(DASICS_JUMPCFG_V << (idx * 16));
    csr_write(0x8c8, jumpcfg); // DasicsJumpCfg
    return 0;
}