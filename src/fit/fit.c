#include "fit.h"
#include <stdlib.h>
#include <asm/unistd.h>
#include <asm/offset.h>

/* Optional: set by apps that link mimalloc-dasics for per-closure heap */
extern void mi_set_ids_dasics(uint32_t library_id, uint32_t closure_id) __attribute__((weak));

fit_entry_t *fit_table = NULL;
static void *current_closure_key = NULL;

void *fit_get_current_closure_key(void) {
    return current_closure_key;
}

int fit_init(uint64_t dasics_funcptr) {
    // Initialize DASICS mechanism
    register_udasics(dasics_funcptr);

    // Initialize the FIT table with static permissions
    return fit_init_static();
}

void fit_destroy(void) {
    fit_entry_t *current, *tmp;

    if (!fit_table) {
        return;
    }

    // Iterate and delete using UTHASH macros
    HASH_ITER(hh, fit_table, current, tmp) {
        HASH_DEL(fit_table, current);  // Remove from hash table

        // Free resources occupied by entry (code_bounds/mem_bounds are inline arrays, no free)
        free(current->syscalls);
        free(current->maincalls);
        free(current);
    }

    fit_table = NULL;  // Set table pointer to NULL

    // Clean up DASICS resources
    unregister_udasics();
}

void fit_print(void) {
    fit_entry_t *current, *tmp;

    if (!fit_table) {
        return;
    }

    // Traverse FIT table and print information
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

uint64_t fit_switchto(void *func, ...) {
    fit_entry_t *entry = NULL;
    uint64_t ret = 0;

    // Find entry in FIT table
    HASH_FIND_PTR(fit_table, &func, entry);  // Use function address as key
    if (!entry) {
        return -1;  // Return error if not found
    }

    // Set current closure key
    current_closure_key = entry->key;

    // Set permissions for current function (handle stored in code_bounds[i].handle / mem_bounds[i].handle)
    uint64_t frame_addr, badfunc_stack_top, badfunc_stack_size;
    for (size_t i = 0; i < entry->code_bounds_num; i++) {
        fit_bounds_t *b = &entry->code_bounds[i];
        b->handle = dasics_jumpcfg_alloc(b->lo, b->hi + 1);
    }
    for (size_t i = 0; i < entry->mem_bounds_num; i++) {
        fit_bounds_t *b = &entry->mem_bounds[i];
        if (b->lo == UINT64_MAX) {
            asm volatile("mv %0, sp" : "=r"(frame_addr));
            badfunc_stack_top = frame_addr - STACK_FRAME_SIZE_LIBCALL;
            badfunc_stack_size = b->hi;
            b->handle = dasics_libcfg_alloc(b->perm,
                                           badfunc_stack_top - badfunc_stack_size,
                                           badfunc_stack_top + 1);
        } else {
            b->handle = dasics_libcfg_alloc(b->perm, b->lo, b->hi + 1);
        }
    }

    if (mi_set_ids_dasics)
        mi_set_ids_dasics(entry->library_id, entry->closure_id);

    // Set argbound permissions if the callback exists
    va_list args;
    va_start(args, func);
    if (entry->argbound_alloc) {
        entry->argbound_alloc(args);
    }
    va_end(args);

    // Execute function
    va_start(args, func);
    ret = lib_call(func, args);
    va_end(args);

    // Remove argbound permissions if the callback exists
    if (entry->argbound_free) {
        entry->argbound_free();
    }

    // Remove allocated permissions (handles stored in code_bounds[i].handle / mem_bounds[i].handle)
    for (size_t i = 0; i < entry->code_bounds_num; i++) {
        dasics_jumpcfg_free(entry->code_bounds[i].handle);
    }
    for (size_t i = 0; i < entry->mem_bounds_num; i++) {
        dasics_libcfg_free(entry->mem_bounds[i].handle);
    }

    // Reset current closure key
    current_closure_key = NULL;

    return ret;
}

int fit_check_syscall(int sysno) {
    // Check if syscall number is valid
    if (sysno < 0 || sysno >= __NR_syscalls) {
        printf("Invalid syscall number %d\n", sysno);
        return -1;  // Return error if invalid syscall number
    }

    // Find entry in FIT table
    fit_entry_t *entry = NULL;
    HASH_FIND_PTR(fit_table, &current_closure_key, entry);  // Use function address as key
    if (!entry) {
        return -1;  // Return error if not found
    }

    // Check if syscall is valid
    if (bitmap_query(entry->syscalls, sysno)) {
        return 0;  // Valid syscall
    } else {
        return -1;  // Invalid syscall
    }
}

int fit_check_maincall(int maincall) {
    // Check if maincall number is valid
    if (maincall < 0 || maincall >= Umaincall_UNKNOWN) {
        printf("Invalid maincall number %d\n", maincall);
        return -1;  // Return error if invalid maincall number
    }

    // Find entry in FIT table
    fit_entry_t *entry = NULL;
    HASH_FIND_PTR(fit_table, &current_closure_key, entry);  // Use function address as key
    if (!entry) {
        return -1;  // Return error if not found
    }

    // Check if maincall is valid
    if (bitmap_query(entry->maincalls, maincall)) {
        return 0;  // Valid maincall
    } else {
        return -1;  // Invalid maincall
    }
}