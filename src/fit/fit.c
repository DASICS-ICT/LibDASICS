/*
 * fit.c - FIT (Function Isolation Table) management.
 *
 * This file owns the FIT hash table and provides the top-level lifecycle
 * APIs (init / destroy) as well as runtime query helpers (print, syscall
 * and maincall checking).
 *
 * Permission granting  -> pgrant.c
 * Domain transitions   -> transition.c
 * Closure-stack state  -> cstack.c
 */
#include "fit.h"
#include "fit_internal.h"
#include <stdlib.h>
#include <asm/unistd.h>

/* ---- FIT hash-table (global, declared extern in fit.h) ---- */
fit_entry_t *fit_table = NULL;

/*
 * fit_init - initialise the whole FIT subsystem.
 *
 * @dasics_funcptr: address of the DASICS handler used by register_udasics().
 *
 * 1. Register the DASICS user-space mechanism.
 * 2. Create the closure stack (trusted base frame).
 * 3. Populate the FIT table from compile-time static data.
 */
int fit_init(uint64_t dasics_funcptr) {
    register_udasics(dasics_funcptr);
    fit_init_closure_stack();
    return fit_init_static();
}

/*
 * fit_destroy - tear down the FIT subsystem and release all resources.
 *
 * Order matters: closure stack first (no more domain switches),
 * then the hash table, and finally the DASICS mechanism itself.
 */
void fit_destroy(void) {
    fit_entry_t *current, *tmp;

    /* 1. Destroy the closure stack */
    fit_destroy_closure_stack();

    /* 2. Free every FIT entry in the hash table */
    if (fit_table) {
        HASH_ITER(hh, fit_table, current, tmp) {
            HASH_DEL(fit_table, current);
            /* code_bounds / mem_bounds are inline arrays, only heap fields need free */
            free(current->syscalls);
            free(current->maincalls);
            free(current);
        }
        fit_table = NULL;
    }

    /* 3. Unregister DASICS */
    unregister_udasics();
}

/* ---- Debug printing ---- */
void fit_print(void) {
    fit_entry_t *current, *tmp;

    if (!fit_table) {
        return;
    }

    // Traverse FIT table and print information for each entry
    HASH_ITER(hh, fit_table, current, tmp) {
        printf("Key: %p\n", current->key);
        printf("Code bounds: %zu\n", current->code_bounds_num);
        for (size_t i = 0; i < current->code_bounds_num; i++) {
            printf("  Bound %zu: perm=0x%x, lo=0x%lx, hi=0x%lx\n",
                   i, current->code_bounds[i].perm,
                   current->code_bounds[i].lo,
                   current->code_bounds[i].hi);
        }
        printf("Mem bounds: %zu\n", current->mem_bounds_num);
        for (size_t i = 0; i < current->mem_bounds_num; i++) {
            printf("  Bound %zu: perm=0x%x, lo=0x%lx, hi=0x%lx\n",
                   i, current->mem_bounds[i].perm,
                   current->mem_bounds[i].lo,
                   current->mem_bounds[i].hi);
        }

        // Print stack information
        printf("Stack: top=0x%lx, size=0x%lx\n",
               (unsigned long)current->stack_top,
               (unsigned long)current->stack_size);

        // Print temporary code bounds
        printf("Temporary code bounds: %zu\n", current->temp_code_bounds_num);
        for (size_t i = 0; i < current->temp_code_bounds_num; i++) {
            printf("  Bound %zu: perm=0x%x, lo=0x%lx, hi=0x%lx\n",
                   i, current->temp_code_bounds[i].perm,
                   current->temp_code_bounds[i].lo,
                   current->temp_code_bounds[i].hi);
        }
        
        // Print temporary mem bounds
        printf("Temporary mem bounds: %zu\n", current->temp_mem_bounds_num);
        for (size_t i = 0; i < current->temp_mem_bounds_num; i++) {
            printf("  Bound %zu: perm=0x%x, lo=0x%lx, hi=0x%lx\n",
                   i, current->temp_mem_bounds[i].perm,
                   current->temp_mem_bounds[i].lo,
                   current->temp_mem_bounds[i].hi);
        }

        // Print temporary times
        printf("Temporary times: %u\n", current->temp_times);

        // Print valid syscall numbers
        printf("Syscalls: ");
        for (size_t i = 0; i < current->syscalls_size * 8; i++) {
            if (bitmap_query(current->syscalls, i)) {
                printf("%zu ", i);
            }
        }
        printf("\n");

        // Print valid maincall numbers
        printf("Maincalls: ");
        for (size_t i = 0; i < current->maincalls_size * 8; i++) {
            if (bitmap_query(current->maincalls, i)) {
                printf("%zu ", i);
            }
        }
        printf("\n");
        printf("--------------------------------------------------\n");
    }
}

/* ---- Runtime permission checks ---- */

/*
 * fit_check_syscall - verify that the current domain is allowed to issue
 * the given system call.
 *
 * Returns 0 if permitted, -1 otherwise.
 */
int fit_check_syscall(int sysno) {
    if (sysno < 0 || sysno >= __NR_syscalls) {
        printf("Invalid syscall number %d\n", sysno);
        return -1;
    }

    void *key = fit_get_current_closure_key();
    fit_entry_t *entry = NULL;
    HASH_FIND_PTR(fit_table, &key, entry);
    if (!entry) {
        return -1;
    }

    return bitmap_query(entry->syscalls, sysno) ? 0 : -1;
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

    void *key = fit_get_current_closure_key();
    fit_entry_t *entry = NULL;
    HASH_FIND_PTR(fit_table, &key, entry);
    if (!entry) {
        return -1;
    }

    return bitmap_query(entry->maincalls, maincall) ? 0 : -1;
}
