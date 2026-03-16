/*
 * transition.c - Domain transition (switch) logic for FIT.
 *
 * A domain transition is the core operation of the FIT mechanism:
 *
 *   1. Push callee's closure onto the stack.
 *   2. Clear the caller's DASICS bounds.
 *   3. Apply the callee's bounds (static + temporary + stack + valist).
 *   4. Invoke the callee via lib_call.
 *   5. On return, pop the stack, clear callee bounds, restore caller bounds.
 *
 * This file also manages the "temporary times" counter which tracks how
 * many transitions a permission grant remains valid for.
 */
#include "fit.h"
#include "fit_internal.h"
#include <stdlib.h>
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
 * do_apply_permission - apply all of an entry's bounds to the DASICS
 * hardware: static code/mem bounds, stack bound, temporary bounds, and
 * the va_list region.
 *
 * The total number of bounds must not exceed the hardware limits
 * (asserted here).
 */
void do_apply_permission(fit_entry_t *entry) {
    assert(entry->code_bounds_num + entry->temp_code_bounds_num <= FIT_CODE_BOUNDS_MAX);
    /* +1 for stack permission, +1 for optional valist */
    assert(entry->mem_bounds_num + entry->temp_mem_bounds_num + 1
           + (entry->valist_size == 0 ? 0 : 1) <= FIT_MEM_BOUNDS_MAX);

    /* Static code & memory bounds */
    apply_bounds(entry->code_bounds_num, entry->mem_bounds_num,
                 entry->code_bounds, entry->mem_bounds);

    /* Stack bound (RW) */
    if (entry->stack_top != 0 && entry->stack_size != 0) {
        dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_W,
                            entry->stack_top - entry->stack_size,
                            entry->stack_top + 1);
    }

    /* Temporary bounds from permission grant */
    if (entry->temp_code_bounds_num > 0 || entry->temp_mem_bounds_num > 0) {
        apply_bounds(entry->temp_code_bounds_num, entry->temp_mem_bounds_num,
                     entry->temp_code_bounds, entry->temp_mem_bounds);
    }

    /* va_list region (RW) */
    if (entry->valist_size > 0) {
        dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_W,
                            entry->valist_base,
                            entry->valist_base + entry->valist_size);
    }
}

/*
 * temp_times_tick - decrement the temporary-grant counter.
 *
 * When it reaches zero the temporary bounds are automatically cleared,
 * so the next transition into this entry will not apply them.
 */
static void temp_times_tick(fit_entry_t *entry) {
    if (entry->temp_times == 0)
        return;
    entry->temp_times--;
    if (entry->temp_times == 0) {
        entry->temp_code_bounds_num = 0;
        entry->temp_mem_bounds_num = 0;
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
    fit_entry_t *entry_callee = NULL;
    HASH_FIND_PTR(fit_table, &func, entry_callee);
    if (!entry_callee) return (uint64_t)-1;

    /* Push callee onto the closure stack */
    if (fit_closure_push(entry_callee->key) != 0)
        return (uint64_t)-1;

    /* Capture the current stack pointer to set the callee's stack bound */
    uint64_t frame_addr;
    asm volatile("mv %0, sp" : "=r"(frame_addr));
    entry_callee->stack_top = frame_addr - STACK_FRAME_SIZE_LIBCALL;

    /* Record va_list base for the callee if it expects variadic arguments */
    if (entry_callee->valist_size > 0) {
        entry_callee->valist_base = (uint64_t)args;
    }

    /* Clear caller's DASICS bounds before entering callee domain */
    dasics_jumpcfg_free_all();
    dasics_libcfg_free_all();

    /* Apply callee's full permission set */
    do_apply_permission(entry_callee);

    /* Notify mimalloc of the active library/closure IDs (if linked) */
    if (mi_set_ids_dasics)
        mi_set_ids_dasics(entry_callee->library_id, entry_callee->closure_id);

    /* ---- Invoke the callee ---- */
    uint64_t ret = lib_call(func, args);

    /* Consume one use of the temporary grant */
    temp_times_tick(entry_callee);

    /* Pop callee from closure stack */
    fit_closure_pop();

    /* Restore previous domain's bounds (or leave cleared for trusted) */
    void *top_key = fit_get_current_closure_key();
    dasics_jumpcfg_free_all();
    dasics_libcfg_free_all();
    if (top_key) {
        fit_entry_t *entry_caller = NULL;
        HASH_FIND_PTR(fit_table, &top_key, entry_caller);
        if (entry_caller)
            do_apply_permission(entry_caller);
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
