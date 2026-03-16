/*
 * cstack.c - Closure stack management for FIT domain transitions.
 *
 * The closure stack is a simple linked-list stack that records the chain
 * of active domains.  The bottom frame (always present after init) has
 * closure_key == NULL, representing the trusted (main) domain.  Every
 * domain switch pushes a new frame; returning from the callee pops it.
 *
 * All stack state is file-local; external code accesses it exclusively
 * through the API declared in fit_internal.h and fit.h.
 */
#include "fit.h"
#include "fit_internal.h"
#include "utstack.h"
#include <stdlib.h>

/* Single frame on the closure stack. */
typedef struct fit_closure_frame {
    void *closure_key;              /* closure key (NULL = trusted domain) */
    struct fit_closure_frame *next; /* utstack intrusive pointer */
} fit_closure_frame_t;

/* The stack itself (file-local). */
static fit_closure_frame_t *closure_stack = NULL;

/*
 * fit_init_closure_stack - push the initial trusted-domain frame.
 *
 * Must be called exactly once during fit_init().  After this call the
 * stack contains one frame with closure_key == NULL.
 */
void fit_init_closure_stack(void) {
    fit_closure_frame_t *f = (fit_closure_frame_t *)malloc(sizeof(fit_closure_frame_t));
    if (!f) return;
    f->closure_key = NULL;
    f->next = NULL;
    closure_stack = f;
}

/*
 * fit_destroy_closure_stack - pop and free every frame.
 *
 * Called from fit_destroy() to release all memory held by the stack.
 */
void fit_destroy_closure_stack(void) {
    fit_closure_frame_t *f;
    while (closure_stack) {
        STACK_POP(closure_stack, f);
        free(f);
    }
}

/*
 * fit_closure_push - push a new frame onto the closure stack.
 *
 * @key: the closure key of the callee being entered.
 * Returns 0 on success, -1 if malloc fails.
 */
int fit_closure_push(void *key) {
    fit_closure_frame_t *frame = (fit_closure_frame_t *)malloc(sizeof(fit_closure_frame_t));
    if (!frame) return -1;
    frame->closure_key = key;
    STACK_PUSH(closure_stack, frame);
    return 0;
}

/*
 * fit_closure_pop - pop and free the top frame.
 *
 * Caller is responsible for ensuring the stack is not empty (i.e. at
 * least the trusted-domain base frame remains).
 */
void fit_closure_pop(void) {
    fit_closure_frame_t *popped = NULL;
    STACK_POP(closure_stack, popped);
    free(popped);
}

/*
 * fit_get_current_closure_key - return the closure key of the current domain.
 *
 * Returns NULL when in the trusted (main) domain, or the key of the
 * innermost active closure otherwise.  Declared in fit.h (public API).
 */
void *fit_get_current_closure_key(void) {
    if (!closure_stack) return NULL;
    return closure_stack->closure_key;
}
