/*
 * compartment.h - Compartment builder API for FIT.
 *
 * Provides a high-level interface for creating and configuring isolation
 * compartments (compartment_t).  Each compartment encapsulates:
 *   - Code and memory bounds (with DASICS permission bits)
 *   - Allowed syscall and maincall bitmaps
 *   - Stack range
 *   - Library/closure IDs for mimalloc integration
 *
 * A compartment is registered into the FIT hash table at creation time
 * (compartment_create), mapped from a function-pointer key.  Multiple
 * keys can share the same compartment via compartment_duplicate().
 *
 * This header is the recommended public interface for application code
 * that builds FIT tables.  It includes fit.h transitively.
 */
#ifndef COMPARTMENT_H
#define COMPARTMENT_H

#include "fit.h"

/*
 * Upper bound for stack size (sanity check).
 * Override at compile time with -DCOMPARTMENT_STACK_SIZE_MAX=... if needed.
 */
#ifndef COMPARTMENT_STACK_SIZE_MAX
#define COMPARTMENT_STACK_SIZE_MAX  (64 * 1024)   /* 64 KiB */
#endif

/*
 * compartment_create - allocate, initialise, and register a new compartment.
 *
 * @func_key:    function-pointer key for the FIT hash table.
 * library_id / closure_id are resolved internally:
 *   - library_id from _get_area(func_key)->fit_library_id
 *   - closure_id from ++_get_area(func_key)->current_closure_id
 * closure_id=0 is reserved for the per-library default compartment.
 *
 * Allocates a compartment_t with ref_count=1, all bounds counts zeroed,
 * syscall/maincall bitmaps set to NULL (lazy allocation), and registers
 * the (func_key -> compartment) mapping in fit_table.
 *
 * Returns the compartment pointer on success, NULL on failure.
 */
compartment_t *compartment_create(void *func_key);

/*
 * compartment_create_default - create a per-library default compartment.
 *
 * Same as compartment_create(), but closure_id is fixed to 0.
 * Used by the dynamic fallback path for unmarked functions.
 */
compartment_t *compartment_create_default(void *func_key);

/*
 * compartment_destroy - unregister and (if last reference) free a compartment.
 *
 * Removes all fit_table entries that point to @comp, then decrements
 * ref_count.  When ref_count reaches zero, the bitmaps and the
 * compartment itself are freed.
 */
void compartment_destroy(compartment_t *comp);

/*
 * compartment_duplicate - register an additional key for an existing compartment.
 *
 * @comp:      the existing compartment to share.
 * @func_key:  the new function-pointer key to map to @comp.
 *
 * Increments comp->ref_count and inserts a new fit_table entry.
 * Returns @comp on success, NULL on failure.
 */
compartment_t *compartment_duplicate(compartment_t *comp, void *func_key);

/*
 * compartment_permit_syscall - allow specific system calls.
 *
 * @comp:  target compartment.
 * @num:   number of syscall IDs that follow.
 * @...:   int-typed syscall numbers (e.g. __NR_getpid, __NR_write).
 *
 * On the first call (when bitmap is NULL), allocates the bitmap.
 * May be called multiple times; permissions accumulate (OR semantics).
 *
 * Returns 0 on success, -1 on error (e.g. allocation failure or invalid ID).
 */
int compartment_permit_syscall(compartment_t *comp, int num, ...);

/*
 * compartment_permit_maincall - allow specific main calls.
 *
 * @comp:  target compartment.
 * @num:   number of maincall IDs that follow.
 * @...:   int-typed maincall numbers (e.g. Umaincall_PRINT, Umaincall_MALLOC).
 *
 * Same lazy-allocation and accumulation semantics as permit_syscall.
 *
 * Returns 0 on success, -1 on error.
 */
int compartment_permit_maincall(compartment_t *comp, int num, ...);

/*
 * compartment_add_code_bound - add an executable code region.
 *
 * @comp:  target compartment.
 * @perm:  permission bits (must include DASICS_LIBCFG_X).
 * @lo:    lower bound address (inclusive).
 * @hi:    upper bound address (inclusive).
 *
 * Skips silently if lo == hi (empty region, returns 0).
 * Returns -1 if lo > hi (invalid) or bounds array is full.
 * Returns 0 on success.
 */
int compartment_add_code_bound(compartment_t *comp, fit_perm_t perm, uint64_t lo, uint64_t hi);

/*
 * compartment_add_mem_bound - add a data/memory access region.
 *
 * @comp:  target compartment.
 * @perm:  permission bits (DASICS_LIBCFG_R, _W combinations).
 * @lo:    lower bound address (inclusive).
 * @hi:    upper bound address (inclusive).
 *
 * Same skip/error semantics as compartment_add_code_bound.
 * Returns 0 on success, -1 on error.
 */
int compartment_add_mem_bound(compartment_t *comp, fit_perm_t perm, uint64_t lo, uint64_t hi);

/*
 * compartment_set_stack - set the stack size for the compartment.
 *
 * @comp:        target compartment.
 * @stack_size:  stack size in bytes (must be <= COMPARTMENT_STACK_SIZE_MAX).
 *
 * stack_top is set to 0 here and will be filled dynamically by
 * transition_pre() at runtime using the current SP.
 *
 * Returns 0 on success, -1 if stack_size exceeds the limit.
 */
int compartment_set_stack(compartment_t *comp, uint64_t stack_size);

#endif /* COMPARTMENT_H */
