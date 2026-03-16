/*
 * pgrant.c - Permission granting for FIT entries.
 *
 * Before a domain transition, the caller may grant temporary permissions
 * (code/memory bounds) to a callee's FIT entry.  These "temp" bounds are
 * applied on top of the entry's static bounds during the next N domain
 * transitions (controlled by the `times` parameter).
 *
 * A grant is only allowed if every bound being granted is a subset of
 * the caller's own bounds (monotonic delegation).  Trusted code
 * (closure_key == NULL) bypasses this check.
 */
#include "fit.h"

/*
 * perm_is_subset_of_caller - check that a single permission is covered
 * by at least one of the caller's static bounds.
 *
 * "Subset" means:
 *   1. [p->lo, p->hi] is contained within [b->lo, b->hi].
 *   2. p->perm bits are a subset of b->perm bits.
 *
 * Code-execute bounds and data bounds are checked against separate arrays.
 */
static int perm_is_subset_of_caller(const fit_bounds_t *p, const fit_entry_t *caller) {
    if (p->perm & DASICS_LIBCFG_X) {
        /* Check against caller's code bounds */
        for (size_t j = 0; j < caller->code_bounds_num; j++) {
            const fit_bounds_t *b = &caller->code_bounds[j];
            if (p->lo >= b->lo && p->hi <= b->hi &&
                (p->perm & ~b->perm) == 0)
                return 1;
        }
    } else {
        /* Check against caller's memory bounds */
        for (size_t j = 0; j < caller->mem_bounds_num; j++) {
            const fit_bounds_t *b = &caller->mem_bounds[j];
            // FIXME: How to represent stack permissions?
            if (b->lo == UINT64_MAX)
                continue;
            if (p->lo >= b->lo && p->hi <= b->hi &&
                (p->perm & ~b->perm) == 0)
                return 1;
        }
    }
    return 0;
}

/*
 * do_permission_grant - core grant logic operating on a known entry.
 *
 * @entry:       target FIT entry to receive the temporary bounds.
 * @perms:       array of bounds to grant.
 * @num:         number of elements in @perms.
 * @valist_size: size of the va_list region to be granted (0 if none).
 * @times:       how many domain transitions the grant stays active for.
 *
 * Only one grant may be active at a time; calling this while a previous
 * grant is still active (temp_times > 0) is an error.
 *
 * Returns 0 on success, -1 on failure.
 */
int do_permission_grant(fit_entry_t *entry, const fit_bounds_t *perms, size_t num, size_t valist_size, unsigned times) {
    /* Only one grant at a time: reject if temp_times is still active. */
    if (entry->temp_times != 0) {
        printf("[FIT] Error: permission grant rejected, previous grant still active (times=%u)\n",
               entry->temp_times);
        return -1;
    }

    /*
     * Retrieve the caller's FIT entry for monotonic-delegation check.
     * If the caller is in the trusted domain (key == NULL), skip the check.
     */
    void *caller_key = fit_get_current_closure_key();
    fit_entry_t *caller = NULL;
    if (caller_key) {
        HASH_FIND_PTR(fit_table, &caller_key, caller);
        if (!caller) {
            printf("[FIT] Error: caller entry not found for key %p\n", caller_key);
            return -1;
        }
    }

    /* Subset check: every granted bound must be covered by the caller. */
    if (caller) {
        for (size_t i = 0; i < num; i++) {
            if (!perm_is_subset_of_caller(&perms[i], caller)) {
                printf("[FIT] Error: permission grant denied, perm[%zu] "
                       "(lo=0x%lx, hi=0x%lx, perm=0x%x) is not a subset "
                       "of caller's bounds\n",
                       i, (unsigned long)perms[i].lo,
                       (unsigned long)perms[i].hi, perms[i].perm);
                return -1;
            }
        }
    }

    /* Verify that adding the granted bounds won't exceed hardware limits. */
    size_t code_count = 0, mem_count = 0;
    for (size_t i = 0; i < num; i++) {
        if (perms[i].perm & DASICS_LIBCFG_X)
            code_count++;
        else
            mem_count++;
    }

    if (code_count + entry->code_bounds_num > FIT_CODE_BOUNDS_MAX) {
        printf("[FIT] Error: code bounds limit exceeded\n");
        return -1;
    }
    if (mem_count + entry->mem_bounds_num > FIT_MEM_BOUNDS_MAX) {
        printf("[FIT] Error: mem bounds limit exceeded\n");
        return -1;
    }

    /* Distribute granted bounds into temp_code_bounds / temp_mem_bounds. */
    entry->temp_code_bounds_num = 0;
    entry->temp_mem_bounds_num = 0;
    for (size_t i = 0; i < num; i++) {
        if (perms[i].perm & DASICS_LIBCFG_X) {
            entry->temp_code_bounds[entry->temp_code_bounds_num++] = perms[i];
        } else {
            entry->temp_mem_bounds[entry->temp_mem_bounds_num++] = perms[i];
        }
    }
    entry->temp_times = times;
    entry->valist_size = valist_size;

    return 0;
}

/*
 * fit_permission_grant - public API: grant temporary permissions by
 * function pointer lookup.
 *
 * Looks up the FIT entry for @func and delegates to do_permission_grant().
 */
int fit_permission_grant(void *func, const fit_bounds_t *perms, size_t num, size_t valist_size, unsigned times) {
    fit_entry_t *entry = NULL;
    HASH_FIND_PTR(fit_table, &func, entry);
    if (!entry) return -1;
    return do_permission_grant(entry, perms, num, valist_size, times);
}
