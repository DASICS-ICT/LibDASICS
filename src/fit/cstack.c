/*
 * cstack.c - Compartment stack management for FIT domain transitions.
 *
 * The compartment stack is a simple linked-list stack that records the
 * chain of active isolation domains.  Each frame stores a compartment_t
 * pointer directly, eliminating the need for a hash-table lookup when
 * the runtime queries the current domain.
 *
 * The bottom frame (always present after init) has comp == NULL,
 * representing the trusted (main) domain.  Every domain switch pushes
 * a new frame; returning from the callee pops it.
 *
 * All stack state is file-local; external code accesses it exclusively
 * through the API declared in fit_internal.h and fit.h.
 */
#include "fit.h"
#include "fit_internal.h"
#include "utstack.h"
#include <stdlib.h>

/* Single frame on the compartment stack. */
typedef struct fit_compartment_frame {
    compartment_t *comp;                    /* compartment pointer (NULL = trusted domain) */
    struct fit_compartment_frame *next;     /* utstack intrusive pointer */
} fit_compartment_frame_t;

/* The stack itself (file-local). */
static fit_compartment_frame_t *compartment_stack = NULL;

/*
 * fit_init_compartment_stack - push the initial trusted-domain frame.
 *
 * Must be called exactly once during fit_init().  After this call the
 * stack contains one frame with comp == NULL.
 */
void fit_init_compartment_stack(void) {
    fit_compartment_frame_t *f = (fit_compartment_frame_t *)malloc(sizeof(fit_compartment_frame_t));
    if (!f) return;
    f->comp = NULL;
    f->next = NULL;
    compartment_stack = f;
}

/*
 * fit_destroy_compartment_stack - pop and free every frame.
 *
 * Called from fit_destroy() to release all memory held by the stack.
 */
void fit_destroy_compartment_stack(void) {
    fit_compartment_frame_t *f;
    while (compartment_stack) {
        STACK_POP(compartment_stack, f);
        free(f);
    }
}

/*
 * fit_compartment_push - push a new frame onto the compartment stack.
 *
 * @comp: the compartment pointer of the callee being entered.
 * Returns 0 on success, -1 if malloc fails.
 */
int fit_compartment_push(compartment_t *comp) {
    fit_compartment_frame_t *frame = (fit_compartment_frame_t *)malloc(sizeof(fit_compartment_frame_t));
    if (!frame) return -1;
    frame->comp = comp;
    STACK_PUSH(compartment_stack, frame);
    return 0;
}

/*
 * fit_compartment_pop - pop and free the top frame.
 *
 * Caller is responsible for ensuring the stack is not empty (i.e. at
 * least the trusted-domain base frame remains).
 */
void fit_compartment_pop(void) {
    fit_compartment_frame_t *popped = NULL;
    STACK_POP(compartment_stack, popped);
    free(popped);
}

/*
 * fit_get_current_compartment - return the compartment of the current
 * execution domain.
 *
 * Returns NULL when in the trusted (main) domain, or the compartment
 * pointer of the innermost active domain otherwise.
 * Declared in fit.h (public API).
 */
compartment_t *fit_get_current_compartment(void) {
    if (!compartment_stack) return NULL;
    return compartment_stack->comp;
}
