#include "fit.h"
#include "utstack.h"
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <asm/unistd.h>
#include <asm/offset.h>
#include <sys/types.h>

/* Optional: set by apps that link mimalloc-dasics for per-closure heap */
extern void __attribute__((weak)) mi_set_ids_dasics(uint32_t library_id, uint32_t closure_id);

/* Closure stack: stack top = current domain; element has closure_key (NULL = trusted). */
typedef struct fit_closure_frame {
    void *closure_key;
    struct fit_closure_frame *next;
} fit_closure_frame_t;

static fit_closure_frame_t *closure_stack = NULL;

static void fit_init_closure_stack(void) {
    fit_closure_frame_t *f = (fit_closure_frame_t *)malloc(sizeof(fit_closure_frame_t));
    if (!f) return;
    f->closure_key = NULL;
    f->next = NULL;
    closure_stack = f;
}

void *fit_get_current_closure_key(void) {
    if (!closure_stack) return NULL;
    return closure_stack->closure_key;
}

fit_entry_t *fit_table = NULL;

int fit_init(uint64_t dasics_funcptr) {
    // Initialize DASICS mechanism
    register_udasics(dasics_funcptr);
    fit_init_closure_stack();
    // Initialize the FIT table with static permissions
    return fit_init_static();
}

void fit_destroy(void) {
    fit_entry_t *current, *tmp;
    fit_closure_frame_t *f;

    while (closure_stack) {
        STACK_POP(closure_stack, f);
        free(f);
    }

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

        // Print stack permission
        printf("Stack: top=0x%lx, size=0x%lx\n", (unsigned long)current->stack_top, (unsigned long)current->stack_size);

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

/*
 * Check that a single permission (lo, hi, perm) is a subset of at least one
 * entry in the caller's code_bounds or mem_bounds (not temp_*).
 * "Subset" means: [lo, hi] is contained within some bound's [b.lo, b.hi],
 * and perm bits are a subset of that bound's perm bits (perm & ~b.perm == 0).
 */
static int perm_is_subset_of_caller(const fit_bounds_t *p, const fit_entry_t *caller) {
    if (p->perm & DASICS_LIBCFG_X) {
        for (size_t j = 0; j < caller->code_bounds_num; j++) {
            const fit_bounds_t *b = &caller->code_bounds[j];
            if (p->lo >= b->lo && p->hi <= b->hi &&
                (p->perm & ~b->perm) == 0)
                return 1;
        }
    } else {
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

int do_permission_grant(fit_entry_t *entry, const fit_bounds_t *perms, size_t num, size_t valist_size, unsigned times) {
    /* Only one grant at a time: reject if temp_times is still active. */
    if (entry->temp_times != 0) {
        printf("[FIT] Error: permission grant rejected, previous grant still active (times=%u)\n",
               entry->temp_times);
        return -1;
    }

    /* Retrieve the caller's entry for subset checking.
     * If caller is trusted (key == NULL), skip subset check. */
    void *caller_key = fit_get_current_closure_key();
    fit_entry_t *caller = NULL;
    if (caller_key) {
        HASH_FIND_PTR(fit_table, &caller_key, caller);
        if (!caller) {
            printf("[FIT] Error: caller entry not found for key %p\n", caller_key);
            return -1;
        }
    }

    /* Subset check: each granted perm must be covered by a caller's bound. */
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

    /* Count code vs mem. */
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

    /* Copy perms into target entry's temp_*_bounds (overwrite, since previous grant is cleared). */
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

int fit_permission_grant(void *func, const fit_bounds_t *perms, size_t num, size_t valist_size, unsigned times) {
    fit_entry_t *entry = NULL;
    HASH_FIND_PTR(fit_table, &func, entry);
    if (!entry) return -1;
    return do_permission_grant(entry, perms, num, valist_size, times);
}

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

void do_apply_permission(fit_entry_t *entry) {
    assert(entry->code_bounds_num + entry->temp_code_bounds_num <= FIT_CODE_BOUNDS_MAX);
    assert(entry->mem_bounds_num + entry->temp_mem_bounds_num + 1 + (entry->valist_size == 0 ? 0 : 1) <= FIT_MEM_BOUNDS_MAX);  // plus one stack permission

    // Apply the code and memory permissions
    apply_bounds(entry->code_bounds_num, entry->mem_bounds_num,
        entry->code_bounds, entry->mem_bounds);

    // Apply the stack permission
    if (entry->stack_top != 0 && entry->stack_size != 0) {
        dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_W, entry->stack_top - entry->stack_size, entry->stack_top + 1);
    }

    // Apply the temporary code and memory permissions
    if (entry->temp_code_bounds_num > 0 || entry->temp_mem_bounds_num > 0) {
        apply_bounds(entry->temp_code_bounds_num, entry->temp_mem_bounds_num,
                     entry->temp_code_bounds, entry->temp_mem_bounds);
    }

    // Apply the valist permission
    if (entry->valist_size > 0) {
        dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_W, entry->valist_base, entry->valist_base + entry->valist_size);
    }
}

/* Decrement temp_times; when it reaches 0, clear all temp bounds. */
static void temp_times_tick(fit_entry_t *entry) {
    if (entry->temp_times == 0)
        return;
    entry->temp_times--;
    if (entry->temp_times == 0) {
        entry->temp_code_bounds_num = 0;
        entry->temp_mem_bounds_num = 0;
    }
}

uint64_t do_transition(void *func, va_list args) {
    fit_entry_t *entry_callee = NULL;
    HASH_FIND_PTR(fit_table, &func, entry_callee);
    if (!entry_callee) return (uint64_t)-1;

    // Record the current closure key to the stack
    fit_closure_frame_t *frame = (fit_closure_frame_t *)malloc(sizeof(fit_closure_frame_t));
    if (!frame) return (uint64_t)-1;
    frame->closure_key = entry_callee->key;
    STACK_PUSH(closure_stack, frame);

    // Set the stack top of current closure
    uint64_t frame_addr;
    asm volatile("mv %0, sp" : "=r"(frame_addr));
    entry_callee->stack_top = frame_addr - STACK_FRAME_SIZE_LIBCALL;

    // Set the va_list base address of current closure
    if (entry_callee->valist_size > 0) {
        entry_callee->valist_base = (uint64_t)args;
    }

    // Clear caller's bounds before entering callee domain
    dasics_jumpcfg_free_all();
    dasics_libcfg_free_all();

    // Apply the permissions of the callee
    do_apply_permission(entry_callee);

    // Set the library and closure ids for mimalloc
    if (mi_set_ids_dasics)
        mi_set_ids_dasics(entry_callee->library_id, entry_callee->closure_id);

    // Invoke callee function
    uint64_t ret = lib_call(func, args);

    // Decrement the temporary times
    temp_times_tick(entry_callee);

    // Pop the closure frame from the stack
    fit_closure_frame_t *popped = NULL;
    STACK_POP(closure_stack, popped);
    free(popped);

    // Restore previous permissions
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

uint64_t fit_switchto(void *func, ...) {
    va_list args;
    va_start(args, func);
    uint64_t ret = do_transition(func, args);
    va_end(args);
    
    return ret;
}

int fit_check_syscall(int sysno) {
    // Check if syscall number is valid
    if (sysno < 0 || sysno >= __NR_syscalls) {
        printf("Invalid syscall number %d\n", sysno);
        return -1;  // Return error if invalid syscall number
    }

    // Find entry in FIT table
    void *key = fit_get_current_closure_key();
    fit_entry_t *entry = NULL;
    HASH_FIND_PTR(fit_table, &key, entry);  // Use function address as key
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
    void *key = fit_get_current_closure_key();
    fit_entry_t *entry = NULL;
    HASH_FIND_PTR(fit_table, &key, entry);  // Use function address as key
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