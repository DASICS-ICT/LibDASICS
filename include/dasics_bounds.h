#ifndef DASICS_BOUNDS_H
#define DASICS_BOUNDS_H

#include <stdint.h>
#include "dasics_isa.h"

/* The API accepts half-open user ranges and rounds outward to the hardware
 * grain. Empty, reversed, or non-user ranges must not become wider grants. */
static inline int dasics_normalize_bound(uint64_t *lo, uint64_t *hi)
{
    if (*hi <= *lo || *hi > DASICS_USER_LIMIT) return -1;
    *lo &= ~(uint64_t)(DASICS_BOUND_GRAIN - 1);
    *hi = (*hi + DASICS_BOUND_GRAIN - 1) & ~(uint64_t)(DASICS_BOUND_GRAIN - 1);
    return 0;
}

#endif
