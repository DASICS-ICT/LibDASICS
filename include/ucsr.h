#ifndef _UCSR_H_
#define _UCSR_H_

#include "dasics_isa.h"

/* Permission changes must not move across the accesses they authorize. */
#define csr_read(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " DASICS_STRINGIFY(reg) : "=r"(__tmp) :: "memory"); \
  __tmp; })

#define csr_write(reg, val) ({ \
  asm volatile ("csrw " DASICS_STRINGIFY(reg) ", %0" :: "rK"(val) : "memory"); })

#endif
