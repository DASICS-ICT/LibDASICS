/*
 * compartment.c - Compartment builder implementation for FIT.
 *
 * This file provides the high-level API for creating, configuring, and
 * managing isolation compartments.  It encapsulates the boilerplate that
 * was previously duplicated across each application's fit_generated.c:
 *
 *   - Memory allocation and zero-initialisation of compartment_t
 *   - Lazy allocation of syscall/maincall bitmaps
 *   - Bounds-array management with limit checking
 *   - Reference-counted sharing via compartment_duplicate()
 *   - Registration / removal from the FIT hash table
 */
#include "compartment.h"
#include "dynamic.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <asm/unistd.h>

/* ======================================================================
 * compartment_create
 * ====================================================================== */
static compartment_t *compartment_create_internal(void *func_key, int is_default) {
    umain_elf_t *elf = _get_area((uint64_t)(uintptr_t)func_key);
    if (!elf) return NULL;

    compartment_t *comp = (compartment_t *)calloc(1, sizeof(compartment_t));
    if (!comp) return NULL;

    comp->elf         = elf;
    comp->library_id  = elf->fit_library_id;
    comp->closure_id  = is_default ? 0 : ++elf->current_closure_id;
    comp->ref_count   = 1;

    /*
     * Bitmaps are left NULL (lazy allocation).
     * All bounds counters are already zero from calloc.
     * stack_top / valist_base are set dynamically by do_transition().
     */

    /* Register this compartment into the FIT hash table. */
    if (fit_map_add(func_key, comp) != 0) {
        free(comp);
        return NULL;
    }

    return comp;
}

compartment_t *compartment_create(void *func_key) {
    return compartment_create_internal(func_key, 0);
}

compartment_t *compartment_create_default(void *func_key) {
    return compartment_create_internal(func_key, 1);
}

/* ======================================================================
 * compartment_destroy
 * ====================================================================== */
void compartment_destroy(compartment_t *comp) {
    if (!comp) return;

    /*
     * Remove all fit_table entries that point to this compartment.
     * There may be more than one if compartment_duplicate() was used.
     */
    fit_entry_t *current, *tmp;
    HASH_ITER(hh, fit_table, current, tmp) {
        if (current->comp == comp) {
            HASH_DEL(fit_table, current);
            free(current);
        }
    }

    /* Decrement reference count; free only when last reference is gone. */
    comp->ref_count--;
    if (comp->ref_count == 0) {
        free(comp->syscalls);
        free(comp->maincalls);
        free(comp);
    }
}

/* ======================================================================
 * compartment_duplicate
 * ====================================================================== */
compartment_t *compartment_duplicate(compartment_t *comp, void *func_key) {
    if (!comp) return NULL;

    /* Register another key pointing to the same compartment. */
    if (fit_map_add(func_key, comp) != 0)
        return NULL;

    comp->ref_count++;
    return comp;
}

/* ======================================================================
 * compartment_permit_syscall
 * ====================================================================== */
int compartment_permit_syscall(compartment_t *comp, int num, ...) {
    if (!comp) return -1;
    if (num <= 0) return 0;

    /* Lazy allocation: create the bitmap on first use. */
    if (!comp->syscalls) {
        comp->syscalls = bitmap_alloc(__NR_syscalls);
        if (!comp->syscalls) return -1;
    }

    va_list ap;
    va_start(ap, num);
    for (int i = 0; i < num; i++) {
        int id = va_arg(ap, int);
        if (id < 0 || id >= __NR_syscalls) {
            printf("[compartment] Warning: invalid syscall id %d, skipping\n", id);
            continue;
        }
        bitmap_set(comp->syscalls, id, true);
    }
    va_end(ap);

    return 0;
}

/* ======================================================================
 * compartment_permit_maincall
 * ====================================================================== */
int compartment_permit_maincall(compartment_t *comp, int num, ...) {
    if (!comp) return -1;
    if (num <= 0) return 0;

    /* Lazy allocation: create the bitmap on first use. */
    if (!comp->maincalls) {
        comp->maincalls = bitmap_alloc(Umaincall_UNKNOWN);
        if (!comp->maincalls) return -1;
    }

    va_list ap;
    va_start(ap, num);
    for (int i = 0; i < num; i++) {
        int id = va_arg(ap, int);
        if (id < 0 || id >= Umaincall_UNKNOWN) {
            printf("[compartment] Warning: invalid maincall id %d, skipping\n", id);
            continue;
        }
        bitmap_set(comp->maincalls, id, true);
    }
    va_end(ap);

    return 0;
}

/* ======================================================================
 * compartment_add_code_bound
 * ====================================================================== */
int compartment_add_code_bound(compartment_t *comp, fit_perm_t perm, uint64_t lo, uint64_t hi) {
    if (!comp) return -1;

    /* Empty region: skip silently (e.g. linker symbols that collapse). */
    if (lo == hi) return 0;

    /* Invalid range check. */
    if (lo > hi) {
        printf("[compartment] Error: code bound lo (0x%lx) > hi (0x%lx)\n",
               (unsigned long)lo, (unsigned long)hi);
        return -1;
    }

    if (comp->code_bounds_num >= FIT_CODE_BOUNDS_MAX) {
        printf("[compartment] Error: code bounds limit (%d) exceeded\n",
               FIT_CODE_BOUNDS_MAX);
        return -1;
    }

    fit_bounds_t *b = &comp->code_bounds[comp->code_bounds_num];
    b->perm   = perm;
    b->lo     = lo;
    b->hi     = hi;
    b->handle = -1;
    comp->code_bounds_num++;

    return 0;
}

/* ======================================================================
 * compartment_add_mem_bound
 * ====================================================================== */
int compartment_add_mem_bound(compartment_t *comp, fit_perm_t perm, uint64_t lo, uint64_t hi) {
    if (!comp) return -1;

    /* Empty region: skip silently (e.g. linker symbols that collapse). */
    if (lo == hi) return 0;

    /* Invalid range check. */
    if (lo > hi) {
        printf("[compartment] Error: mem bound lo (0x%lx) > hi (0x%lx)\n",
               (unsigned long)lo, (unsigned long)hi);
        return -1;
    }

    if (comp->mem_bounds_num >= FIT_MEM_BOUNDS_MAX) {
        printf("[compartment] Error: mem bounds limit (%d) exceeded\n",
               FIT_MEM_BOUNDS_MAX);
        return -1;
    }

    fit_bounds_t *b = &comp->mem_bounds[comp->mem_bounds_num];
    b->perm   = perm;
    b->lo     = lo;
    b->hi     = hi;
    b->handle = -1;
    comp->mem_bounds_num++;

    return 0;
}

/* ======================================================================
 * compartment_set_stack
 * ====================================================================== */
int compartment_set_stack(compartment_t *comp, uint64_t stack_size) {
    if (!comp) return -1;

    if (stack_size > COMPARTMENT_STACK_SIZE_MAX) {
        printf("[compartment] Error: stack_size (0x%lx) exceeds max (0x%lx)\n",
               (unsigned long)stack_size, (unsigned long)COMPARTMENT_STACK_SIZE_MAX);
        return -1;
    }

    /* stack_top is set to 0 here; it will be filled dynamically by
     * do_transition() using the current SP at domain-switch time. */
    comp->stack_top  = 0;
    comp->stack_size = stack_size;

    return 0;
}

/* ======================================================================
 * compartment_set_valist
 * ====================================================================== */
int compartment_set_valist(compartment_t *comp, size_t valist_size) {
    if (!comp) return -1;

    if (valist_size > COMPARTMENT_VALIST_SIZE_MAX) {
        printf("[compartment] Error: valist_size (0x%zx) exceeds max (0x%lx)\n",
               valist_size, (unsigned long)COMPARTMENT_VALIST_SIZE_MAX);
        return -1;
    }

    /* valist_base is set dynamically by do_transition(). */
    comp->valist_size = valist_size;

    return 0;
}
