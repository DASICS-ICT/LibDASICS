#ifndef DASICS_ISA_H
#define DASICS_ISA_H

/* NanhuV5-FDI architectural constants shared by C and assembly. Main-config
 * masks describe the implemented CSR bank, not internal checker-only bits. */
#ifdef __ASSEMBLER__
#define DASICS_UL(value) value
#else
#define DASICS_UL(value) value##UL
#endif

#define CSR_USTATUS                0x000
#define CSR_UIE                    0x004
#define CSR_UTVEC                  0x005
#define CSR_USCRATCH               0x040
#define CSR_UEPC                   0x041
#define CSR_UCAUSE                 0x042
#define CSR_UTVAL                  0x043
#define CSR_UIP                    0x044
#define CSR_UTIMER                 0x045
#define CSR_SFETCHCTL              0x9e0
#define CSR_DUMCFG                 0x9e1
#define CSR_DUMBOUNDLO             0x9e2
#define CSR_DUMBOUNDHI             0x9e3
#define CSR_DSMCFG                 0xbc0
#define CSR_DSMBOUNDLO             0xbc2
#define CSR_DSMBOUNDHI             0xbc3
#define CSR_DLCFG                  0x880
#define CSR_DLBOUND0LO             0x890
#define CSR_DLBOUND0HI             0x891
#define CSR_DLBOUND1LO             0x892
#define CSR_DLBOUND1HI             0x893
#define CSR_DLBOUND2LO             0x894
#define CSR_DLBOUND2HI             0x895
#define CSR_DLBOUND3LO             0x896
#define CSR_DLBOUND3HI             0x897
#define CSR_DLBOUND4LO             0x898
#define CSR_DLBOUND4HI             0x899
#define CSR_DLBOUND5LO             0x89a
#define CSR_DLBOUND5HI             0x89b
#define CSR_DLBOUND6LO             0x89c
#define CSR_DLBOUND6HI             0x89d
#define CSR_DLBOUND7LO             0x89e
#define CSR_DLBOUND7HI             0x89f
#define CSR_DLBOUND8LO             0x8a0
#define CSR_DLBOUND8HI             0x8a1
#define CSR_DLBOUND9LO             0x8a2
#define CSR_DLBOUND9HI             0x8a3
#define CSR_DLBOUND10LO            0x8a4
#define CSR_DLBOUND10HI            0x8a5
#define CSR_DLBOUND11LO            0x8a6
#define CSR_DLBOUND11HI            0x8a7
#define CSR_DLBOUND12LO            0x8a8
#define CSR_DLBOUND12HI            0x8a9
#define CSR_DLBOUND13LO            0x8aa
#define CSR_DLBOUND13HI            0x8ab
#define CSR_DLBOUND14LO            0x8ac
#define CSR_DLBOUND14HI            0x8ad
#define CSR_DLBOUND15LO            0x8ae
#define CSR_DLBOUND15HI            0x8af
#define CSR_DMAINCALL              0x8b0
#define CSR_DRETURNPC              0x8b1
#define CSR_DFZRETURN              0x8b2
#define CSR_DFREASON               0x8b3
#define CSR_DJBOUND0LO             0x8c0
#define CSR_DJBOUND0HI             0x8c1
#define CSR_DJBOUND1LO             0x8c2
#define CSR_DJBOUND1HI             0x8c3
#define CSR_DJBOUND2LO             0x8c4
#define CSR_DJBOUND2HI             0x8c5
#define CSR_DJBOUND3LO             0x8c6
#define CSR_DJBOUND3HI             0x8c7
#define CSR_DJCFG                  0x8c8

#define DASICS_SMAINCFG_MASK DASICS_UL(0x3ff)
#define DASICS_UMAINCFG_MASK DASICS_UL(0x3e)
#define DASICS_MAINCFG_MASK DASICS_SMAINCFG_MASK
#define DASICS_SCFG_ENA DASICS_UL(0x1)
#define DASICS_UCFG_ENA DASICS_UL(0x2)
#define DASICS_UCFG_CUET DASICS_UL(0x4)
#define DASICS_UCFG_CUST DASICS_UL(0x8)
#define DASICS_UCFG_CULT DASICS_UL(0x10)
#define DASICS_UCFG_CUFT DASICS_UL(0x20)
#define DASICS_SCFG_CSET DASICS_UL(0x40)
#define DASICS_SCFG_CSST DASICS_UL(0x80)
#define DASICS_SCFG_CSLT DASICS_UL(0x100)
#define DASICS_SCFG_CSFT DASICS_UL(0x200)
#define DASICS_UCFG_CLS DASICS_UL(0x3c)
#define DASICS_SCFG_CLS DASICS_UL(0x3c0)

#define DASICS_LIBCFG_WIDTH 16
#define DASICS_LIBCFG_MASK DASICS_UL(0xf)
#define DASICS_LIBCFG_V DASICS_UL(0x8)
#define DASICS_LIBCFG_R DASICS_UL(0x2)
#define DASICS_LIBCFG_W DASICS_UL(0x1)
#define DASICS_JUMPCFG_WIDTH 4
#define DASICS_JUMPCFG_MASK DASICS_UL(0xffff)
#define DASICS_JUMPCFG_V DASICS_UL(0x1)
#define DASICS_BOUND_GRAIN 8
#define DASICS_CACHE_BLOCK_BYTES 64
#define DASICS_USER_LIMIT DASICS_UL(0x800000000000)
#define DASICS_LIBRARY_BASE (DASICS_USER_LIMIT >> 1)

#define EXC_DASICS_UCHECK_FAULT 24
#define DFR_NO_DASICS_FAULT 0
#define DFR_ECALL_DASICS_FAULT 1
#define DFR_LOAD_DASICS_FAULT 2
#define DFR_STORE_DASICS_FAULT 3
#define DFR_JUMP_DASICS_FAULT 4
#define DFR_LOAD_MPK_FAULT 5
#define DFR_STORE_MPK_FAULT 6
#define DASICS_UTIMER_IRQ DASICS_UL(0x10)
#define DASICS_UTIMER_CAUSE DASICS_UL(0x8000000000000004)

/* Retain each call's operand fields while using the custom3 major opcode. */
#define DASICS_FDICALL_JR_RA_T0 0x000290fb
#define DASICS_FDICALL_JR_RA_T2 0x000390fb
#define DASICS_FDICALL_JR_RA_A0 0x000510fb

#define DASICS_STRINGIFY_RAW(value) #value
#define DASICS_STRINGIFY(value) DASICS_STRINGIFY_RAW(value)

#endif
