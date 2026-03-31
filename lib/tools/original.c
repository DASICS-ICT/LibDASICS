#include <dasics_start.h>
#include <ucsr.h>
#include <udasics.h>

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


int32_t original_libcfg_alloc(uint64_t cfg, uint64_t lo, uint64_t hi) {
    uint64_t libcfg = csr_read(dlcfg);
    int32_t max_cfgs = DASICS_LIBCFG_WIDTH;
    int32_t step = 4;

    for (int32_t idx = 0; idx < max_cfgs; ++idx) {
        uint64_t curr_cfg = (libcfg >> (idx * step)) & DASICS_LIBCFG_MASK;

        if ((curr_cfg & DASICS_LIBCFG_V) == 0)  // Found available config
        {
            // Write DASICS bounds csr
            LIBBOUND_WRITE(hi, lo, idx);

            // Write config
            libcfg &= ~(DASICS_LIBCFG_MASK << (idx * step));
            libcfg |= (cfg & DASICS_LIBCFG_MASK) << (idx * step);
            csr_write(dlcfg, libcfg);

            return idx;
        }
    }

    return -1;
}

int32_t original_libcfg_free(int32_t idx) {
    if (idx < 0 || idx >= DASICS_LIBCFG_WIDTH) return -1;

    int32_t step = 4;
    uint64_t libcfg = csr_read(dlcfg);
    libcfg &= ~(DASICS_LIBCFG_V << (idx * step));
    csr_write(dlcfg, libcfg);
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
    uint64_t libcfg = csr_read(dlcfg);
    return (libcfg >> (idx * step)) & DASICS_LIBCFG_MASK;
}

int32_t original_jumpcfg_alloc(uint64_t lo, uint64_t hi)
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

int32_t original_jumpcfg_free(int32_t idx) {
    if (idx < 0 || idx >= DASICS_JUMPCFG_WIDTH) {
        return -1;
    }

    int32_t step = 16;
    uint64_t jumpcfg = csr_read(djcfg);
    jumpcfg &= ~(DASICS_JUMPCFG_V << (idx * step));
    csr_write(djcfg, jumpcfg);
    return 0;
}

int32_t original_jumpcfg_free_all()
{
    for (int idx = 0; idx < DASICS_JUMPCFG_WIDTH; idx++)
    {
        original_jumpcfg_free(idx);
    }
    return 0;
}
