#ifndef FIT_H
#define FIT_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include "udasics.h"   // Contains DASICS permission definitions and Umaincall_UNKNOWN
#include "uthash.h"    // UTHash library
#include "bitmap.h"    // Bitmap operations

// Permission type (using definitions from udasics.h)
typedef uint32_t fit_perm_t;

// Maximum number of code bounds and memory bounds per entry (overridable via -D)
#ifndef FIT_CODE_BOUNDS_MAX
#define FIT_CODE_BOUNDS_MAX 4
#endif
#ifndef FIT_MEM_BOUNDS_MAX
#define FIT_MEM_BOUNDS_MAX 16
#endif

// Single memory bound definition
typedef struct fit_bounds {
    fit_perm_t perm;   // Permission bits, using DASICS_LIBCFG_XX series definitions
    uint64_t lo;       // Lower bound address
    uint64_t hi;       // Upper bound address
    int32_t handle;   // DASICS libcfg/jumpcfg handle (filled in fit_switchto or by Umaincall_MALLOC)
} fit_bounds_t;

/* Optional handle array type (e.g. for argbound dynamic bounds) */
typedef struct fit_handles {
    fit_perm_t perm;
    int handle;
} fit_handles_t;

// FIT table entry structure (must contain UTHash's hh field)
typedef struct fit_entry {
    void *key;                                    // Hash key (function address)

    fit_bounds_t code_bounds[FIT_CODE_BOUNDS_MAX]; // Code (executable) bounds
    size_t code_bounds_num;                        // Number of code bounds
    fit_bounds_t mem_bounds[FIT_MEM_BOUNDS_MAX];   // Data/memory bounds
    size_t mem_bounds_num;                         // Number of mem bounds

    // Stack permission
    uint64_t stack_top;       // Stack top address
    uint64_t stack_size;      // Stack size

    /* Temporary granted bounds: single grant at a time, shared times counter. */
    fit_bounds_t temp_code_bounds[FIT_CODE_BOUNDS_MAX];
    size_t temp_code_bounds_num;
    fit_bounds_t temp_mem_bounds[FIT_MEM_BOUNDS_MAX];
    size_t temp_mem_bounds_num;
    unsigned temp_times;

    // va_list permission
    uint64_t valist_base;        // Base address of the va_list
    size_t valist_size;          // Size of the va_list

    uint32_t library_id;         // Library id for mimalloc (e.g. 0 = user program)
    uint32_t closure_id;         // Closure id for mimalloc (per-function)
    int heap_alloc_done;         // 1 if self-managed heap bound was added (for Umaincall_MALLOC scheme B)

    uint8_t *syscalls;            // System call bitmap
    size_t syscalls_size;         // System call bitmap size (in bytes)

    uint8_t *maincalls;           // Main call bitmap
    size_t maincalls_size;        // Main call bitmap size (in bytes)

    UT_hash_handle hh;            // Required field for UTHash
} fit_entry_t;

extern fit_entry_t *fit_table;    // FIT table pointer

// Function declarations
extern int fit_init(uint64_t dasics_funcptr);
extern int fit_init_static(void);
extern void fit_destroy(void);
extern void fit_print(void);
extern uint64_t fit_switchto(void *func, ...);
extern int fit_check_syscall(int sysno);
extern int fit_check_maincall(int maincall);
extern void *fit_get_current_closure_key(void);

/* Permission grant: copy perms into target entry's temp_*_bounds (4/16 checked). */
extern int do_permission_grant(fit_entry_t *entry, const fit_bounds_t *perms, size_t num, size_t valist_size, unsigned times);
extern int fit_permission_grant(void *func, const fit_bounds_t *perms, size_t num, size_t valist_size, unsigned times);

/* Apply entry's code/mem + temp bounds to DASICS. */
extern void do_apply_permission(fit_entry_t *entry);
/* Domain switch: push, apply B, lib_call, then pop and restore (free_all + apply prev or free_all). */
extern uint64_t do_transition(void *func, va_list args);

#endif // FIT_H