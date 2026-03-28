/*
 * transition.c - Domain transition (switch) logic for FIT.
 *
 * A domain transition is the core operation of the FIT mechanism:
 *
 *   1. Push callee's compartment onto the compartment stack.
 *   2. Clear the caller's DASICS bounds.
 *   3. Apply the callee's bounds (static + temporary + stack + valist).
 *   4. Invoke the callee via lib_call (FIT path) or lib_call_context (PLT path).
 *   5. On return, pop the stack, clear callee bounds, restore caller bounds.
 *
 * Two entry points share the same pre/post logic:
 *   - do_transition()         : FIT explicit path (wrapper(va_list) convention)
 *   - do_transition_dynamic() : PLT dynamic call path (raw a0-a7 register context)
 *
 * This file also manages the "temporary times" counter which tracks how
 * many transitions a permission grant remains valid for.
 */
#include "fit.h"
#include "fit_internal.h"
#include "compartment.h"
#include "dynamic.h"
#include <assert.h>
#include <string.h>
#include <asm/unistd.h>
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

/*
 * resolve_plt_target - normalize a possible func@plt address to the
 * corresponding real target address.
 *
 * Strategy: check the current domain's ELF first (fast path), then
 * fall back to scanning all ELFs (slow path).  The fast path hits in
 * the common case where the PLT stub belongs to the caller's own ELF.
 *
 * PLT address ranges are non-overlapping across ELFs, so scanning is
 * always deterministic.  This replaces the old caller_elf parameter
 * approach, which was incorrect when the trusted domain contained
 * multiple ELFs (e.g. main program + trusted third-party libraries)
 * -- the old code always fell back to _umain_elf_table (main program),
 * causing PLT resolution failures for trusted libraries whose PLT
 * ranges differ from the main program's.
 *
 * If @func does not fall inside any known ELF's PLT, it is not a PLT
 * stub and is returned unchanged.
 */
static void *resolve_plt_target(void *func) {
    if (!_umain_elf_table)
        return func;

    uint64_t addr = (uint64_t)(uintptr_t)func;
    int plt_idx;

    /* Fast path: try the current domain's ELF first. */
    compartment_t *current = fit_get_current_compartment();
    umain_elf_t *current_elf = current ? current->elf : _umain_elf_table;

    if (current_elf) {
        plt_idx = _is_plt_area(addr, current_elf);
        if (plt_idx != NOEXIST)
            return (void *)(uintptr_t)current_elf->_local_got_table[plt_idx + 2];
    }

    /* Slow path: scan all ELFs, skipping current_elf (already checked). */
    umain_elf_t *elf = _umain_elf_table;
    do {
        if (elf != current_elf) {
            plt_idx = _is_plt_area(addr, elf);
            if (plt_idx != NOEXIST)
                return (void *)(uintptr_t)elf->_local_got_table[plt_idx + 2];
        }
        elf = elf->umain_elf_next;
    } while (elf != _umain_elf_table);

    return func;
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

    /*
     * If func is a PLT stub, normalize it to the real target address
     * before lib_call().  resolve_plt_target() scans all ELFs
     * automatically, so this works regardless of whether the caller is
     * the main program, a trusted third-party library, or an untrusted
     * compartment.
     */
    void *real_func = resolve_plt_target(func);

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

    /* Switch mimalloc to the callee's self-managed heap */
    mi_set_ids_dasics(callee->library_id, callee->closure_id);

    /* ---- Invoke the callee ---- */
    /*
     * lib_call() takes an explicit va_list object as its second parameter.
     *
     * This call style is deliberate:
     * - We do not try to re-expand caller-side "..." arguments at this layer.
     * - The transition target is expected to be a wrapper function that accepts
     *   va_list directly (for example, `int func_wrapper(va_list args)`).
     * - That wrapper then materializes concrete arguments with va_arg().
     *
     * Current limitation:
     * - The low-level forwarding stub still uses a register-limited ABI path.
     * - Wider argument forwarding is deferred to future compiler-side work.
     */
    uint64_t ret = lib_call(real_func, args);

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

    /* Restore mimalloc heap IDs for the caller domain */
    if (caller) {
        mi_set_ids_dasics(caller->library_id, caller->closure_id);
    } else {
        mi_set_ids_dasics(0, 0);
    }

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
/* ---- PLT dynamic call transition ---- */

/*
 * lazy_create_default_compartment - create and cache a whole-library
 * compartment for an unmarked function.
 *
 * Called when fit_find(func) returns NULL during a PLT cross-library call.
 * If the library already has a cached default_compartment, the new function
 * key is registered as an alias (compartment_duplicate).  Otherwise a brand
 * new compartment is built with permissions mirroring the old cross_call
 * logic:
 *
 *   code:  [_plt_start, _plt_end) and [_text_start, _text_end)  executable
 *   data:  [_r_start, _r_end)       read-only
 *          [_w_start, _w_end)       read-write
 *   stack: 16 * PAGE_SIZE           read-write (top set at transition time)
 *   syscall/maincall:               all permitted (bitmap filled with 0xFF)
 *
 * The compartment is registered in the FIT table with func as key, and
 * cached in lib->default_compartment for future calls.
 *
 * closure_id is fixed to 0 for default compartments (fine-grained marked
 * compartments use closure_id >= 1).
 */
static compartment_t *lazy_create_default_compartment(void *func,
                                                       umain_elf_t *lib) {
    /* Fast path: library already has a default compartment -- just alias. */
    if (lib->default_compartment) {
        compartment_t *dup = compartment_duplicate(lib->default_compartment, func);
        if (!dup) {
            printf("[FIT] FATAL: compartment_duplicate failed for %p\n", func);
            while (1);
        }
        return lib->default_compartment;
    }

    /* Slow path: first unmarked call into this library -- build from scratch. */
    compartment_t *comp = compartment_create_default(func);
    if (!comp) {
        printf("[FIT] FATAL: compartment_create failed for default comp (lib=%s)\n",
               lib->real_name);
        while (1);
    }

    /* Executable: .plt stubs and .text* (separate bounds; layout varies by linker). */
    compartment_add_code_bound(comp, DASICS_LIBCFG_X,
                                   lib->_plt_start, lib->_plt_end);
    compartment_add_code_bound(comp, DASICS_LIBCFG_X,
                                   lib->_text_start, lib->_text_end);

    /* Read-only data region (.rodata, .eh_frame, etc.). */
    compartment_add_mem_bound(comp, DASICS_LIBCFG_R | DASICS_LIBCFG_V,
                              lib->_r_start, lib->_r_end);

    /* Read-write data region (.data + .bss, includes GOT). */
    compartment_add_mem_bound(comp, DASICS_LIBCFG_R | DASICS_LIBCFG_W | DASICS_LIBCFG_V,
                              lib->_w_start, lib->_w_end);

    /* Stack: 16 pages, same size as the old cross_call path. */
    compartment_set_stack(comp, 16 * PAGE_SIZE);

    /*
     * Syscall and maincall bitmaps: fully open.
     * Allocate bitmaps and fill with 0xFF so every call ID is permitted.
     * This preserves the pre-FIT behaviour where untrusted libraries had
     * no syscall/maincall filtering.
     */
    comp->syscalls = bitmap_alloc(__NR_syscalls);
    if (!comp->syscalls) {
        printf("[FIT] FATAL: bitmap_alloc(syscalls) failed\n");
        while (1);
    }
    memset(comp->syscalls, 0xFF, (__NR_syscalls + 7) / 8);

    comp->maincalls = bitmap_alloc(Umaincall_UNKNOWN);
    if (!comp->maincalls) {
        printf("[FIT] FATAL: bitmap_alloc(maincalls) failed\n");
        while (1);
    }
    memset(comp->maincalls, 0xFF, (Umaincall_UNKNOWN + 7) / 8);

    /* Cache for subsequent unmarked calls into the same library. */
    lib->default_compartment = comp;
    return comp;
}

/*
 * do_transition_dynamic - perform a full domain switch for a PLT-intercepted
 * cross-library call.
 *
 * This is the PLT counterpart of do_transition().  Both share the same
 * pre/post logic (push, clear, apply, pop, restore); the only difference
 * is the call stub:
 *   - do_transition       uses lib_call   (wrapper receives va_list)
 *   - do_transition_dynamic uses lib_call_context (target gets raw a0-a7)
 *
 * @func:        resolved address of the target function.
 * @saved_regs:  pointer to the saved {a0, a1, ..., a7} array on the
 *               assembly-level stack frame (8 consecutive uint64_t).
 * @target_elf:  umain_elf_t of the library containing @func.  Passed
 *               explicitly from assembly to skip runtime _get_area lookup.
 *
 * Returns the uint64_t value returned by the callee.
 */
uint64_t do_transition_dynamic(void *func, uint64_t *saved_regs,
                               umain_elf_t *target_elf) {
    /*
     * Step 1: Resolve the callee compartment.
     * Marked functions (registered via fit_generated.c) take absolute priority.
     * Unmarked functions fall back to the per-library default compartment.
     */
    compartment_t *callee = fit_find(func);
    if (!callee) {
        callee = lazy_create_default_compartment(func, target_elf);
    }

    /* Step 2: Push callee domain onto the compartment stack. */
    if (fit_compartment_push(callee) != 0) {
        printf("[FIT] FATAL: fit_compartment_push failed\n");
        while (1);
    }

    /* Capture current SP to set the callee's stack bound. */
    uint64_t frame_addr;
    asm volatile("mv %0, sp" : "=r"(frame_addr));
    callee->stack_top = frame_addr - STACK_FRAME_SIZE_LIBCALL;

    /* Step 3: Clear all caller bounds before entering callee domain. */
    dasics_jumpcfg_free_all();
    dasics_libcfg_free_all();

    /* Step 4: Apply callee's full permission set. */
    do_apply_permission(callee);

    /* Step 5: Switch mimalloc heap to the callee's isolated arena. */
    mi_set_ids_dasics(callee->library_id, callee->closure_id);

    /*
     * Step 6: Invoke the target function with the original caller arguments.
     * lib_call_context restores a0-a7 from saved_regs and jumps via
     * dasicscall.jr -- the target returns naturally (no ra hijacking).
     */
    uint64_t ret = lib_call_context(func, saved_regs);

    /* Step 7: Consume one use of any temporary permission grant. */
    temp_times_tick(callee);

    /* Step 8: Pop callee from compartment stack. */
    fit_compartment_pop();

    /*
     * Step 9: Restore the previous domain's mimalloc heap and bounds.
     * fit_get_current_compartment() reads from the stack -- no hash lookup.
     */
    compartment_t *caller = fit_get_current_compartment();
    if (caller)
        mi_set_ids_dasics(caller->library_id, caller->closure_id);
    else
        mi_set_ids_dasics(0, 0);

    dasics_jumpcfg_free_all();
    dasics_libcfg_free_all();
    if (caller)
        do_apply_permission(caller);

    return ret;
}

