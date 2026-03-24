/*
 * fit.h - Public header for the FIT (Function Isolation Table) module.
 *
 * Core types:
 *   compartment_t  - Isolation compartment holding permissions, bounds,
 *                    syscall/maincall bitmaps, stack/valist ranges, etc.
 *   fit_entry_t    - Hash-table entry mapping a function-pointer key to
 *                    a compartment_t.  Multiple entries may share the
 *                    same compartment (see compartment_duplicate).
 *   fit_bounds_t   - A single address-range bound with permission bits.
 *
 * The FIT table (fit_table) is a UTHash table of fit_entry_t nodes.
 */
#ifndef FIT_H
#define FIT_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include "udasics.h"   // Contains DASICS permission definitions and Umaincall_UNKNOWN
#include "uthash.h"    // UTHash library
#include "bitmap.h"    // Bitmap operations

/* ======================================================================
 * Permission type (using definitions from udasics.h, e.g. DASICS_LIBCFG_R)
 * ====================================================================== */
typedef uint32_t fit_perm_t;

/* Maximum number of code bounds and memory bounds per compartment (overridable via -D) */
#ifndef FIT_CODE_BOUNDS_MAX
#define FIT_CODE_BOUNDS_MAX 4
#endif
#ifndef FIT_MEM_BOUNDS_MAX
#define FIT_MEM_BOUNDS_MAX 16
#endif

/* ======================================================================
 * Single address-range bound definition
 * ====================================================================== */
typedef struct fit_bounds {
    fit_perm_t perm;   // Permission bits, using DASICS_LIBCFG_XX series definitions
    uint64_t lo;       // Lower bound address
    uint64_t hi;       // Upper bound address
    int32_t handle;    // DASICS libcfg/jumpcfg handle (filled at domain switch or by Umaincall_MALLOC)
} fit_bounds_t;

/* Optional handle array type (e.g. for argbound dynamic bounds) */
typedef struct fit_handles {
    fit_perm_t perm;
    int handle;
} fit_handles_t;

/* ======================================================================
 * compartment_t - Isolation compartment data.
 *
 * Decoupled from the hash table: key and hh live in fit_entry_t (the
 * hash-table wrapper).  A ref_count field enables multiple fit_entry_t
 * nodes to share the same compartment via compartment_duplicate().
 * ====================================================================== */
typedef struct compartment {
    fit_bounds_t code_bounds[FIT_CODE_BOUNDS_MAX]; // Code (executable) bounds
    size_t code_bounds_num;                        // Number of code bounds
    fit_bounds_t mem_bounds[FIT_MEM_BOUNDS_MAX];   // Data/memory bounds
    size_t mem_bounds_num;                         // Number of mem bounds

    // Stack permission
    uint64_t stack_top;       // Stack top address (set dynamically by do_transition)
    uint64_t stack_size;      // Stack size

    /* Temporary granted bounds: single grant at a time, shared times counter. */
    fit_bounds_t temp_code_bounds[FIT_CODE_BOUNDS_MAX];
    size_t temp_code_bounds_num;
    fit_bounds_t temp_mem_bounds[FIT_MEM_BOUNDS_MAX];
    size_t temp_mem_bounds_num;
    unsigned temp_times;

    // va_list permission
    uint64_t valist_base;        // Base address of the va_list (set dynamically)
    size_t valist_size;          // Size of the va_list

    struct umain_elf *elf;       // Owning ELF module (NULL for trusted domain sentinel)
    uint32_t library_id;         // Cached library id for mimalloc (from elf->fit_library_id)
    uint32_t closure_id;         // Cached closure id for mimalloc (allocated per compartment)
    int heap_alloc_done;         // 1 if self-managed heap bound was added (for Umaincall_MALLOC scheme B)

    /*
     * Syscall / maincall bitmaps.
     * NULL means "no permission at all" (lazy allocation: allocated on
     * first compartment_permit_syscall / compartment_permit_maincall call).
     */
    uint8_t *syscalls;            // System call bitmap (NULL = none allowed)
    uint8_t *maincalls;           // Main call bitmap (NULL = none allowed)

    /*
     * Reference count for shared compartments.
     * Incremented by compartment_duplicate(), decremented by compartment_destroy().
     * The compartment is freed when ref_count reaches zero.
     */
    uint32_t ref_count;
} compartment_t;

/* ======================================================================
 * fit_entry_t - FIT hash-table entry.
 *
 * Each entry maps a function-pointer key to a compartment.
 * Multiple entries may point to the same compartment_t when
 * compartment_duplicate() is used.
 * ====================================================================== */
typedef struct fit_entry {
    void *key;              // Hash key (function address)
    compartment_t *comp;    // Pointer to the isolation compartment
    UT_hash_handle hh;      // Required field for UTHash
} fit_entry_t;

/* ---- FIT hash-table (global) ---- */
extern fit_entry_t *fit_table;

/* ======================================================================
 * Inline helpers for hash-table operations.
 * These minimise changes in call-sites that previously used raw
 * HASH_FIND_PTR / HASH_ADD_PTR on the old monolithic fit_entry_t.
 * ====================================================================== */

/*
 * fit_find - look up a compartment by function-pointer key.
 * Returns the compartment pointer, or NULL if not found.
 */
static inline compartment_t *fit_find(void *key) {
    fit_entry_t *m = NULL;
    HASH_FIND_PTR(fit_table, &key, m);
    return m ? m->comp : NULL;
}

/*
 * fit_map_add - insert a (key -> compartment) mapping into fit_table.
 * Allocates a new fit_entry_t wrapper.  Returns 0 on success, -1 on failure.
 */
static inline int fit_map_add(void *key, compartment_t *comp) {
    fit_entry_t *m = (fit_entry_t *)malloc(sizeof(fit_entry_t));
    if (!m) return -1;
    m->key = key;
    m->comp = comp;
    HASH_ADD_PTR(fit_table, key, m);
    return 0;
}

/* ======================================================================
 * Public function declarations
 * ====================================================================== */
extern int fit_init(uint64_t dasics_funcptr);
extern int fit_init_static(void);
extern int fit_init_dynamic(void);
extern void fit_destroy(void);
extern void fit_print(void);
extern uint64_t fit_switchto(void *func, ...);
extern int fit_check_syscall(int sysno);
extern int fit_check_maincall(int maincall);

/*
 * fit_get_current_compartment - return the compartment of the current
 * execution domain from the compartment stack.
 *
 * Returns NULL when in the trusted (main) domain.
 */
extern compartment_t *fit_get_current_compartment(void);

/* Permission grant: copy perms into target compartment's temp_*_bounds (4/16 checked). */
extern int do_permission_grant(compartment_t *comp, const fit_bounds_t *perms, size_t num, size_t valist_size, unsigned times);
extern int fit_permission_grant(void *func, const fit_bounds_t *perms, size_t num, size_t valist_size, unsigned times);

/* Apply compartment's code/mem + temp bounds to DASICS hardware. */
extern void do_apply_permission(compartment_t *comp);
/* Domain switch: push, apply callee bounds, lib_call, then pop and restore caller bounds. */
extern uint64_t do_transition(void *func, va_list args);

/*
 * do_transition_dynamic - PLT dynamic call transition.
 *
 * Performs the same push/apply/pop/restore sequence as do_transition(),
 * but uses lib_call_context (raw a0-a7) instead of lib_call (va_list).
 * If the function has no FIT entry, a default whole-library compartment
 * is lazily created and cached on target_elf->default_compartment.
 *
 * @func:        resolved target function address.
 * @saved_regs:  pointer to saved {a0..a7} from the PLT intercept frame.
 * @target_elf:  the umain_elf_t of the target library (passed from assembly
 *               to avoid runtime _get_area lookup overhead).
 *
 * Returns the callee's uint64_t return value.
 */
struct umain_elf;
extern uint64_t do_transition_dynamic(void *func, uint64_t *saved_regs,
                                      struct umain_elf *target_elf);

#endif // FIT_H
