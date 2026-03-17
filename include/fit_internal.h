/*
 * fit_internal.h - Internal header for the FIT (Function Isolation Table) module.
 *
 * This header declares internal APIs shared across fit.c, cstack.c,
 * pgrant.c, transition.c, and compartment.c.  It is NOT part of the
 * public interface; application code should only include fit.h (or
 * compartment.h which includes fit.h).
 */
#ifndef FIT_INTERNAL_H
#define FIT_INTERNAL_H

#include <stdint.h>
#include "fit.h"

/* ========================================================================
 * Compartment-stack management (implemented in cstack.c)
 *
 * The compartment stack tracks the current execution domain by storing
 * compartment_t pointers directly, eliminating the need for a
 * hash-table lookup when querying the active domain.
 *
 * The bottom frame always represents the trusted (main) domain with
 * comp == NULL.  Each domain transition pushes a new frame; returning
 * pops it.
 * ======================================================================== */

/* Initialise the compartment stack (push the initial trusted frame). */
void fit_init_compartment_stack(void);

/* Destroy and free every frame on the compartment stack. */
void fit_destroy_compartment_stack(void);

/* Push a new frame with the given compartment pointer onto the stack.
 * Returns 0 on success, -1 on allocation failure. */
int fit_compartment_push(compartment_t *comp);

/* Pop the top frame from the stack and free it. */
void fit_compartment_pop(void);

/* ========================================================================
 * mimalloc integration
 *
 * LibDASICS requires mimalloc-dasics at link time.  The standard
 * malloc/free are overridden by mimalloc; only mi_set_ids_dasics
 * needs an explicit call to switch heap IDs on domain transitions.
 * ======================================================================== */
#include <mimalloc.h>

#endif /* FIT_INTERNAL_H */
