#include "fit.h"
#include <stdlib.h>
#include <asm/unistd.h>
#include <asm/offset.h>

fit_entry_t *fit_table = NULL;
static void *current_closure_key = NULL;

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

        // Free resources occupied by entry
        free(current->bounds_data);
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
        printf("Bounds: %zu\n", current->bounds_num);
        for (size_t i = 0; i < current->bounds_num; i++) {
            printf("  Bound %zu: perm=0x%x, lo=0x%lx, hi=0x%lx\n",
                   i, current->bounds_data[i].perm,
                   current->bounds_data[i].lo,
                   current->bounds_data[i].hi);
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

    // Set permissions for current function
    uint64_t frame_addr, badfunc_stack_top, badfunc_stack_size;
    fit_handles_t *handle_array = (fit_handles_t *)malloc(entry->bounds_num * sizeof(fit_handles_t));
    for (size_t i = 0; i < entry->bounds_num; i++) {
        handle_array[i].perm = entry->bounds_data[i].perm;
        if (entry->bounds_data[i].perm & DASICS_LIBCFG_X) {
            handle_array[i].handle = dasics_jumpcfg_alloc(entry->bounds_data[i].lo,
                                                          entry->bounds_data[i].hi + 1);
        } else if (entry->bounds_data[i].lo == UINT64_MAX) {
            asm volatile("mv %0, sp" : "=r"(frame_addr));
            badfunc_stack_top = frame_addr - STACK_FRAME_SIZE_LIBCALL;
            badfunc_stack_size = entry->bounds_data[i].hi;
            handle_array[i].handle = dasics_libcfg_alloc(entry->bounds_data[i].perm,
                                                         badfunc_stack_top - badfunc_stack_size,
                                                         badfunc_stack_top + 1);
        } else {
            handle_array[i].handle = dasics_libcfg_alloc(entry->bounds_data[i].perm,
                                                         entry->bounds_data[i].lo,
                                                         entry->bounds_data[i].hi + 1);
        }
    }

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

    // Remove allocated permissions
    for (size_t i = 0; i < entry->bounds_num; i++) {
        if (handle_array[i].perm & DASICS_LIBCFG_X) {
            dasics_jumpcfg_free(handle_array[i].handle);
        } else {
            dasics_libcfg_free(handle_array[i].handle);
        }
    }
    free(handle_array);

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