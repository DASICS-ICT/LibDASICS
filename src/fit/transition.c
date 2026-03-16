/*
 * transition.c - Domain transition (switch) logic for FIT.
 *
 * A domain transition is the core operation of the FIT mechanism:
 *
 *   1. Push callee's compartment onto the compartment stack.
 *   2. Clear the caller's DASICS bounds.
 *   3. Apply the callee's bounds (static + temporary + stack + valist).
 *   4. Invoke the callee via lib_call.
 *   5. On return, pop the stack, clear callee bounds, restore caller bounds.
 *
 * This file also manages the "temporary times" counter which tracks how
 * many transitions a permission grant remains valid for.
 *
 * Key change: the compartment stack now stores compartment_t pointers
 * directly, so the caller-restore path no longer needs a hash lookup.
 */
#include "fit.h"
#include "fit_internal.h"
#include <assert.h>
#include <asm/offset.h>

/* ---- Helpers ---- */

/*
 * apply_bounds - programme an array of code and memory bounds into the
 * DASICS hardware.
 *
 * Code bounds go to jumpcfg registers; memory bounds go to libcfg
 * registers.  Hardware addresses are [lo, hi+1) (exclusive upper bound).
 */
static void apply_bounds(size_t code_num, size_t mem_num,
                         fit_bounds_t *code_bounds, fit_bounds_t *mem_bounds) {
    for (size_t i = 0; i < code_num; i++) {
        fit_bounds_t *b = &code_bounds[i];
        dasics_jumpcfg_alloc(b->lo, b->hi + 1);
    }
    for (size_t i = 0; i < mem_num; i++) {
        fit_bounds_t *b = &mem_bounds[i];
        dasics_libcfg_alloc(b->perm, b->lo, b->hi + 1);
    }
}

/*
 * do_apply_permission - apply all of a compartment's bounds to the DASICS
 * hardware: static code/mem bounds, stack bound, temporary bounds, and
 * the va_list region.
 *
 * The total number of bounds must not exceed the hardware limits
 * (asserted here).
 */
void do_apply_permission(compartment_t *comp) {
    assert(comp->code_bounds_num + comp->temp_code_bounds_num <= FIT_CODE_BOUNDS_MAX);
    /* +1 for stack permission, +1 for optional valist */
    assert(comp->mem_bounds_num + comp->temp_mem_bounds_num + 1
           + (comp->valist_size == 0 ? 0 : 1) <= FIT_MEM_BOUNDS_MAX);

    /* Static code & memory bounds */
    apply_bounds(comp->code_bounds_num, comp->mem_bounds_num,
                 comp->code_bounds, comp->mem_bounds);

    /* Stack bound (RW) */
    if (comp->stack_top != 0 && comp->stack_size != 0) {
        dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_W,
                            comp->stack_top - comp->stack_size,
                            comp->stack_top + 1);
    }

    /* Temporary bounds from permission grant */
    if (comp->temp_code_bounds_num > 0 || comp->temp_mem_bounds_num > 0) {
        apply_bounds(comp->temp_code_bounds_num, comp->temp_mem_bounds_num,
                     comp->temp_code_bounds, comp->temp_mem_bounds);
    }

    /* va_list region (RW) */
    if (comp->valist_size > 0) {
        dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_W,
                            comp->valist_base,
                            comp->valist_base + comp->valist_size);
    }
}

/*
 * temp_times_tick - decrement the temporary-grant counter.
 *
 * When it reaches zero the temporary bounds are automatically cleared,
 * so the next transition into this compartment will not apply them.
 */
static void temp_times_tick(compartment_t *comp) {
    if (comp->temp_times == 0)
        return;
    comp->temp_times--;
    if (comp->temp_times == 0) {
        comp->temp_code_bounds_num = 0;
        comp->temp_mem_bounds_num = 0;
    }
}

/* ---- Domain transition ---- */

/*
 * do_transition - perform a full domain switch to the function @func.
 *
 * @func: target function pointer (must have a FIT entry).
 * @args: forwarded va_list from fit_switchto().
 *
 * Returns the callee's uint64_t return value, or (uint64_t)-1 on error.
 */
uint64_t do_transition(void *func, va_list args) {
    /* Look up the target compartment by function pointer (hash lookup needed here). */
    compartment_t *callee = fit_find(func);
    if (!callee) return (uint64_t)-1;

    /* Push callee compartment onto the compartment stack */
    if (fit_compartment_push(callee) != 0)
        return (uint64_t)-1;

    /* Capture the current stack pointer to set the callee's stack bound */
    uint64_t frame_addr;
    asm volatile("mv %0, sp" : "=r"(frame_addr));
    callee->stack_top = frame_addr - STACK_FRAME_SIZE_LIBCALL;

    /* Record va_list base for the callee if it expects variadic arguments */
    if (callee->valist_size > 0) {
        callee->valist_base = (uint64_t)args;
    }

    /* Clear caller's DASICS bounds before entering callee domain */
    dasics_jumpcfg_free_all();
    dasics_libcfg_free_all();

    /* Apply callee's full permission set */
    do_apply_permission(callee);

    /* Notify mimalloc of the active library/closure IDs (if linked) */
    if (mi_set_ids_dasics)
        mi_set_ids_dasics(callee->library_id, callee->closure_id);

    /* ---- Invoke the callee ---- */
    uint64_t ret = lib_call(func, args);

    /* Consume one use of the temporary grant */
    temp_times_tick(callee);

    /* Pop callee from compartment stack */
    fit_compartment_pop();

    /*
     * Restore previous domain's bounds.
     * fit_get_current_compartment() returns the caller compartment
     * directly from the stack -- no hash lookup needed.
     */
    compartment_t *caller = fit_get_current_compartment();
    dasics_jumpcfg_free_all();
    dasics_libcfg_free_all();
    if (caller) {
        do_apply_permission(caller);
    }
    return ret;
}

/*
 * fit_switchto - public variadic entry point for domain transitions.
 *
 * Usage: uint64_t ret = fit_switchto(func_ptr, arg1, arg2, ...);
 */
uint64_t fit_switchto(void *func, ...) {
    va_list args;
    va_start(args, func);
    uint64_t ret = do_transition(func, args);
    va_end(args);

    return ret;
}
