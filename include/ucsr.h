#ifndef _UCSR_H_
#define _UCSR_H_

/* DASICS Main cfg bitmasks */
#define DASICS_MAINCFG_MASK 0xfUL
#define DASICS_UCFG_CLS     0x8UL
#define DASICS_SCFG_CLS     0x4UL
#define DASICS_UCFG_ENA     0x2UL
#define DASICS_SCFG_ENA     0x1UL

/*
 * CSR access helpers.
 * Pass LLVM-recognized CSR names directly, e.g. csr_read(dlcfg).
 *
 * N extension CSRs:
 *   ustatus uie utvec uscratch uepc ucause utval uip
 *
 * DASICS CSRs:
 *   dumcfg dumbound0 dumbound1
 *   dlcfg dlbound0..dlbound31
 *   dmaincall dretpc dretpcactz dfreason
 *   djbound0..djbound7 djcfg
 */
#define csr_read(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define csr_write(reg, val) ({ \
  asm volatile ("csrw " #reg ", %0" :: "rK"(val)); })

#endif
