/*
 * fit_internal.h - Internal header for the FIT (Function Isolation Table) module.
 *
 * This header declares internal APIs shared across fit.c, cstack.c,
 * pgrant.c, and transition.c.  It is NOT part of the public interface;
 * application code should only include fit.h.
 */
#ifndef FIT_INTERNAL_H
#define FIT_INTERNAL_H

#include <stdint.h>

/* ========================================================================
 * Closure-stack management (implemented in cstack.c)
 *
 * The closure stack tracks the current execution domain.  The bottom frame
 * always represents the trusted (main) domain with key == NULL.  Each
 * domain transition pushes a new frame; returning pops it.
 * ======================================================================== */

/* Initialise the closure stack (push the initial trusted frame). */
void fit_init_closure_stack(void);

/* Destroy and free every frame on the closure stack. */
void fit_destroy_closure_stack(void);

/* Push a new frame with the given closure key onto the stack. Returns 0 on
 * success, -1 on allocation failure. */
int fit_closure_push(void *key);

/* Pop the top frame from the stack and free it. */
void fit_closure_pop(void);

/* ========================================================================
 * Optional mimalloc integration (weak symbol)
 *
 * If the application links mimalloc-dasics, this symbol resolves to the
 * real function; otherwise it stays NULL and callers must check before use.
 * ======================================================================== */
extern void __attribute__((weak)) mi_set_ids_dasics(uint32_t library_id,
                                                    uint32_t closure_id);

#endif /* FIT_INTERNAL_H */
