#include <dasics_start.h>
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
            default: \
                printf("\x1b[31m%s\x1b[0m","[DASICS]Error: out of libound register range\n"); \
        }


int32_t original_libcfg_alloc(uint64_t cfg, uint64_t lo, uint64_t hi) {
    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
    int32_t step = 4;

    for (int32_t idx = 0; idx < max_cfgs; ++idx) {
        uint64_t curr_cfg = (libcfg >> (idx * step)) & DASICS_LIBCFG_MASK;

        if ((curr_cfg & DASICS_LIBCFG_V) == 0)  // Found available config
        {
            // Write DASICS bounds csr
            LIBBOUND_LOOKUP(hi, lo, idx, WRITE);

            // Write config
            libcfg &= ~(DASICS_LIBCFG_MASK << (idx * step));
            libcfg |= (cfg & DASICS_LIBCFG_MASK) << (idx * step);
            csr_write(0x880, libcfg);   // DasicsLibCfg

            return idx;
        }
    }

    return -1;
}

int32_t original_libcfg_free(int32_t idx) {
    if (idx < 0 || idx >= DASICS_LIBCFG_WIDTH) return -1;

    int32_t step = 4;
    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
    libcfg &= ~(DASICS_LIBCFG_V << (idx * step));
    csr_write(0x880, libcfg);   // DasicsLibCfg
    return 0;
}

int32_t original_libcfg_free_all()
{
    int32_t idx = 0;

    for (; idx < DASICS_LIBCFG_WIDTH; ++idx) {
        original_libcfg_free(idx);
    }   
    return 0;
}

int32_t original_libcfg_get(int32_t idx) {
    if (idx < 0 || idx >= DASICS_LIBCFG_WIDTH) return -1;

    int32_t step = 4;
    uint64_t libcfg = csr_read(0x880);  // DasicsLibCfg
    return (libcfg >> (idx * step)) & DASICS_LIBCFG_MASK;
}

int32_t original_jumpcfg_alloc(uint64_t lo, uint64_t hi)
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
                default:
                    break;
            }

            jumpcfg &= ~(DASICS_JUMPCFG_MASK << (idx * step));
            jumpcfg |= DASICS_JUMPCFG_V << (idx * step);
            csr_write(0x8c8, jumpcfg); // DasicsJumpCfg
            asm("fence.i");
            return idx;
        }
    }

    return -1;
}

int32_t original_jumpcfg_free(int32_t idx) {
    if (idx < 0 || idx >= DASICS_JUMPCFG_WIDTH) {
        return -1;
    }

    int32_t step = 16;
    uint64_t jumpcfg = csr_read(0x8c8);    // DasicsJumpCfg
    jumpcfg &= ~(DASICS_JUMPCFG_V << (idx * step));
    csr_write(0x8c8, jumpcfg); // DasicsJumpCfg
    asm("fence.i");
    return 0;
}

int32_t original_jumpcfg_free_all()
{
    for (int idx = 0; idx < DASICS_JUMPCFG_WIDTH; idx++)
    {
        original_jumpcfg_free(idx);
    }
    asm("fence.i");
    return 0;
}
