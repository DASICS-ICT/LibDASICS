#include <stdlib.h>
#include <utrap.h>
#include <stdio.h>
#include <dasics_stdio.h>
#include <ucsr.h>
#include <udasics.h>

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

static void print_dasics_mem_bound(int32_t idx, uint64_t libcfg, uint64_t memlevel)
{
    uint64_t idxcfg = ((libcfg >> (4 * idx)) & DASICS_LIBCFG_MASK);
    char * cfg  = NULL;
    // if ((idxcfg & DASICS_LIBCFG_V) == 0) return;
    cfg = "null";
    if ((idxcfg & DASICS_LIBCFG_R) && (idxcfg & DASICS_LIBCFG_W))
        cfg = "Read/Write";
    else if ((idxcfg & DASICS_LIBCFG_R))
        cfg = "Read";
    else if ((idxcfg & DASICS_LIBCFG_W))
        cfg = "Write";

    uint64_t level = ((memlevel >> (2 * idx)) & 0x3); 
    uint64_t hi, lo;
    LIBBOUND_LOOKUP(hi, lo, idx, READ);

    dasics_printf("[BOUND%d]: lo: 0x%lx, hi: 0x%lx, memlevel: %d, cfg: %s\n", idx, lo, hi, level, cfg);
}

static void print_dasics_jump_bound(int32_t idx, uint64_t jmpcfg, uint64_t jmplevel)
{
    uint64_t idxcfg = ((jmpcfg >> (4 * idx)) & 0xful);

    if (!(idxcfg & DASICS_JUMPCFG_V)) return;
    uint64_t level = ((jmplevel >> (2 * idx)) & 0x3ul);
    uint64_t hi, lo;
    switch (idx) {
        case 0:
            lo = csr_read(0x8c0);  // DasicsJumpBound0Lo
            hi = csr_read(0x8c1);  // DasicsJumpBound0Hi
            break;
        case 1:
            lo = csr_read(0x8c2);  // DasicsJumpBound1Lo
            hi = csr_read(0x8c3);  // DasicsJumpBound1Hi
            break;
        case 2:
            lo = csr_read(0x8c4);  // DasicsJumpBound2Lo
            hi = csr_read(0x8c5);  // DasicsJumpBound2Hi
            break;
        case 3:
            lo = csr_read(0x8c6);  // DasicsJumpBound3Lo
            hi = csr_read(0x8c7);  // DasicsJumpBound3Hi
            break;
        default:
            break;
    }    

    dasics_printf("[BOUND%d]: lo: 0x%lx, hi: 0x%lx, jmplevel: %d\n", idx, lo, hi, level);
}


void print_dasics_csr()
{

    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
    uint64_t memlevel = csr_read(0x8cc); // DasicsMemLevel
    dasics_printf("[DASICS_EXCEPTION]: MEM bound\n");
    dasics_printf("libcfg: 0x%lx, memlevel: 0x%lx\n",libcfg, memlevel);
    for (int i = 0; i < DASICS_LIBCFG_WIDTH; i++)
    {
        print_dasics_mem_bound(i, libcfg, memlevel);
    }
    
    uint64_t jumpcfg = csr_read(0x8c8);
    uint64_t jumplevel = csr_read(0x8cd);    // DasicsJumpLevel
    dasics_printf("jumpcfg: 0x%lx, jumplevel: 0x%lx\n",jumpcfg, jumplevel);
    dasics_printf("[DASICS_EXCEPTION]: JUMP bound\n");
    for (int i = 0; i < DASICS_JUMPCFG_WIDTH; i++)
    {
        print_dasics_jump_bound(i, jumpcfg, jumplevel);
    }
}

/*
 * This function is used to deal with the DasicsULoadFault
 * error return -1
 */
int handle_DasicsULoadFault(struct ucontext_trap * regs)
{
    int idx = 0;
    uint16_t c_ld;
    uint32_t _ld;

//     umain_got_t * _got_entry = _get_area(regs->uepc);
// #ifdef DASICS_DEBUG
//     if (_got_entry)
//         dasics_printf("[ufault info]: hit read ufault uepc: 0x%lx, address: 0x%lx \n", regs->uepc, regs->utval);
//     // debug_print_umain_map(_got_entry);
// #endif
//     if (_got_entry == NULL)
//     {
//     #ifdef DASICS_DEBUG
//         dasics_printf("[error]: failed to find the _got_entry\n");
//     #endif
//         if (_umain_got_table == NULL) goto dasics_static_load;
//         exit(1);
//     }
    dasics_printf("[DASICS_EXCEPTION]: Load fault: 0x%lx, pc: %lx\n", regs->utval, regs->uepc);


    print_dasics_csr();

dasics_static_load:
    

    #define LD_INCMASK 0x00000003

    // Jump load instruction for future excution
    c_ld = *((uint16_t *)regs->uepc);
    _ld = *((uint32_t *)regs->uepc);

    if ((_ld & LD_INCMASK) == LD_INCMASK)
        regs->uepc += 4;
    else 
        regs->uepc += 2;

    
    if (idx == -1)
    {
        dasics_printf("[error]: no more libbounds!!\n");
        exit(1);
    }    
    return 0;
}


/*
 * This function is used to deal with the DasicsULoadFault  
 * error return -1
 */
int handle_DasicsUStoreFault(struct ucontext_trap * regs)
{
    int idx = 0;
    uint16_t c_sd;
    uint32_t _sd;
//     umain_got_t * _got_entry = _get_area(regs->uepc);
// // #ifdef DASICS_DEBUG
//     dasics_printf("[ufault info]: hit write ufault uepc: 0x%lx, address: 0x%lx \n", regs->uepc, regs->utval);
//     // debug_print_umain_map(_got_entry);
// // #endif
//     if (_got_entry == NULL)
//     {
//     #ifdef DASICS_DEBUG
//         dasics_printf("[error]: failed to find the _got_entry\n");
//     #endif
//         if (_umain_got_table == NULL) goto dasics_static_store;
//         exit(1);
//     }
    printf("[DASICS_EXCEPTION]: Store fault: 0x%lx pc: %lx\n", regs->utval, regs->uepc);


    print_dasics_csr();

dasics_static_store:
    #define SD_INCMASK 0x00000003

    // Jump load instruction for future excution
    c_sd = *((uint16_t *)regs->uepc);
    _sd = *((uint32_t *)regs->uepc);

    if ((_sd & SD_INCMASK) == SD_INCMASK)
        regs->uepc += 4;
    else 
        regs->uepc += 2;


    if (idx == -1)
    {
        dasics_printf("[error]: no more libbounds!!\n");
        exit(1);
    }    
    return 0;
}
 