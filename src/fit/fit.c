/*
 * fit.c - FIT (Function Isolation Table) management.
 *
 * This file owns the FIT hash table and provides the top-level lifecycle
 * APIs (init / destroy) as well as runtime query helpers (print, syscall
 * and maincall checking).
 *
 * The hash table now uses an indirect layout:
 *   fit_entry_t (key + hh)  -->  compartment_t (permissions, bounds, ...)
 *
 * Multiple fit_entry_t nodes may share one compartment_t (via
 * compartment_duplicate).  Each compartment now owns an intrusive list of
 * its entries and is freed when that list becomes empty.
 *
 * Permission granting  -> pgrant.c
 * Domain transitions   -> transition.c
 * Compartment stack    -> cstack.c
 * Compartment builder  -> compartment.c
 */
#include "fit.h"
#include "fit_internal.h"
#include <stdlib.h>
#include <asm/unistd.h>

/* ---- FIT hash-table (global, declared extern in fit.h) ---- */
fit_entry_t *fit_table = NULL;

static int fit_initialized = 0;

/*
 * Weak fallback: application may override fit_init_static().
 * If not overridden, static FIT registration is treated as empty.
 */
int __attribute__((weak)) fit_init_static(void) {
#ifdef DASICS_DEBUG
    static int warned = 0;
    if (!warned) {
        warned = 1;
        printf("[WARNING] fit_init_static weak fallback used (no generated static fit init found)\n");
    }
#endif
    return 0;
}

/*
 * Weak fallback: application may override fit_init_dynamic().
 * If not overridden, dynamic FIT registration is treated as empty.
 */
int __attribute__((weak)) fit_init_dynamic(void) {
#ifdef DASICS_DEBUG
    static int warned = 0;
    if (!warned) {
        warned = 1;
        printf("[WARNING] fit_init_dynamic weak fallback used (no generated dynamic fit init found)\n");
    }
#endif
    return 0;
}

/*
 * fit_init - initialise the whole FIT subsystem (GCC constructor).
 *
 * Called automatically before main() via __attribute__((constructor)).
 * Uses the default dasics_umaincall_helper as the maincall handler.
 *
 * 1. Register the DASICS user-space mechanism with default handler.
 * 2. Create the compartment stack (trusted base frame).
 * 3. Populate the FIT table from compile-time static data.
 */
__attribute__((constructor))
int fit_init(void) {
    if (fit_initialized) return 0;
    fit_initialized = 1;

    int ret;

    register_udasics();
    fit_init_compartment_stack();

    /* Stage 1: program/static-library generated FIT registration. */
    ret = fit_init_static();
    if (ret != 0)
        return ret;

    /* Stage 2: dynamic-library generated FIT registration (aggregated). */
    ret = fit_init_dynamic();
    if (ret != 0)
        return ret;

    return 0;
}

/*
 * fit_destroy - tear down the FIT subsystem and release all resources
 *               (GCC destructor / cleanup stage).
 *
 * Called automatically after main() returns via __attribute__((destructor)).
 *
 * Order matters: compartment stack first (no more domain switches),
 * then the hash table, and finally the DASICS mechanism itself.
 *
 * For each hash-table entry we remove it from its compartment's entry list.
 * When the last entry disappears, we free the compartment and its bitmaps.
 */
 __attribute__((destructor))
 void fit_destroy(void) {
     if (!fit_initialized) return;
     fit_initialized = 0;
 
     fit_entry_t *current, *tmp;
 
     /* 1. Destroy the compartment stack */
     fit_destroy_compartment_stack();
 
     /* 2. Free every FIT entry in the hash table */
     if (fit_table) {
         HASH_ITER(hh, fit_table, current, tmp) {
             HASH_DEL(fit_table, current);
             compartment_t *comp = current->comp;
             if (comp) {
                 DL_DELETE2(comp->entries, current, comp_prev, comp_next);
                 if (!comp || comp->entries != NULL) {
                    return;
                }
            
                free(comp->syscalls);
                free(comp->maincalls);
                free(comp);
             }
             /* Free the hash-table wrapper entry */
             free(current);
         }
         fit_table = NULL;
     }
 
     /* 3. Unregister DASICS */
     unregister_udasics();
 }

/* ---- Debug printing ---- */
static void fit_print_compartment_detail(const compartment_t *c) {
    fit_entry_t *entry;

    printf("Keys:");
    DL_FOREACH2(c->entries, entry, comp_next) {
        printf(" %p", entry->key);
    }
    printf("\n");

    printf("Code bounds: %zu\n", c->code_bounds_num);
    for (size_t i = 0; i < c->code_bounds_num; i++) {
        printf("  Bound %zu: perm=0x%x, lo=0x%lx, hi=0x%lx\n",
               i, c->code_bounds[i].perm,
               c->code_bounds[i].lo,
               c->code_bounds[i].hi);
    }
    printf("Mem bounds: %zu\n", c->mem_bounds_num);
    for (size_t i = 0; i < c->mem_bounds_num; i++) {
        printf("  Bound %zu: perm=0x%x, lo=0x%lx, hi=0x%lx\n",
               i, c->mem_bounds[i].perm,
               c->mem_bounds[i].lo,
               c->mem_bounds[i].hi);
    }

    // Print stack information
    printf("Stack: top=0x%lx, size=0x%lx\n",
           (unsigned long)c->stack_top,
           (unsigned long)c->stack_size);

    // Print temporary code bounds
    printf("Temporary code bounds: %zu\n", c->temp_code_bounds_num);
    for (size_t i = 0; i < c->temp_code_bounds_num; i++) {
        printf("  Bound %zu: perm=0x%x, lo=0x%lx, hi=0x%lx\n",
               i, c->temp_code_bounds[i].perm,
               c->temp_code_bounds[i].lo,
               c->temp_code_bounds[i].hi);
    }

    // Print temporary mem bounds
    printf("Temporary mem bounds: %zu\n", c->temp_mem_bounds_num);
    for (size_t i = 0; i < c->temp_mem_bounds_num; i++) {
        printf("  Bound %zu: perm=0x%x, lo=0x%lx, hi=0x%lx\n",
               i, c->temp_mem_bounds[i].perm,
               c->temp_mem_bounds[i].lo,
               c->temp_mem_bounds[i].hi);
    }

    // Print temporary times
    printf("Temporary times: %u\n", c->temp_times);

    // Print valid syscall numbers
    printf("Syscalls: ");
    if (c->syscalls) {
        for (size_t i = 0; i < __NR_syscalls; i++) {
            if (bitmap_query(c->syscalls, i)) {
                printf("%zu ", i);
            }
        }
    } else {
        printf("(none)");
    }
    printf("\n");

    // Print valid maincall numbers
    printf("Maincalls: ");
    if (c->maincalls) {
        for (size_t i = 0; i < Umaincall_UNKNOWN; i++) {
            if (bitmap_query(c->maincalls, i)) {
                printf("%zu ", i);
            }
        }
    } else {
        printf("(none)");
    }
    printf("\n");
}

void fit_print(void) {
    fit_entry_t *current, *tmp;

    if (!fit_table) {
        return;
    }

    HASH_ITER(hh, fit_table, current, tmp) {
        compartment_t *c = current->comp;
        if (!c) continue;
        if (current != c->entries) continue;  // Avoid printing the same compartment multiple times

        fit_print_compartment_detail(c);
        printf("--------------------------------------------------\n");
    }
}

/* ---- Runtime permission checks ---- */

/*
 * fit_check_syscall - verify that the current domain is allowed to issue
 * the given system call.
 *
 * Now uses fit_get_current_compartment() to obtain the compartment
 * pointer directly from the compartment stack (no hash lookup needed).
 *
 * Returns 0 if permitted, -1 otherwise.
 */
int fit_check_syscall(int sysno) {
    if (sysno < 0 || sysno >= __NR_syscalls) {
        printf("Invalid syscall number %d\n", sysno);
        return -1;
    }

    compartment_t *comp = fit_get_current_compartment();
    if (!comp) {
        return -1;
    }

    /* NULL bitmap means no syscalls are allowed (lazy allocation). */
    if (!comp->syscalls) {
        return -1;
    }

    return bitmap_query(comp->syscalls, sysno) ? 0 : -1;
}

/*
 * fit_check_maincall - verify that the current domain is allowed to issue
 * the given main-call.
 *
 * Returns 0 if permitted, -1 otherwise.
 */
int fit_check_maincall(int maincall) {
    if (maincall < 0 || maincall >= Umaincall_UNKNOWN) {
        printf("Invalid maincall number %d\n", maincall);
        return -1;
    }

    compartment_t *comp = fit_get_current_compartment();
    if (!comp) {
        return -1;
    }

    /* NULL bitmap means no maincalls are allowed (lazy allocation). */
    if (!comp->maincalls) {
        return -1;
    }

    return bitmap_query(comp->maincalls, maincall) ? 0 : -1;
}
