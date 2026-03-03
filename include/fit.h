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

// Single memory bound definition
typedef struct fit_bounds {
    fit_perm_t perm;   // Permission bits, using DASICS_LIBCFG_XX series definitions
    uint64_t lo;       // Lower bound address
    uint64_t hi;       // Upper bound address
} fit_bounds_t;

typedef struct fit_handles {
    fit_perm_t perm;   // Permission bits, using DASICS_LIBCFG_XX series definitions
    int handle;        // Handle
} fit_handles_t;

// FIT table entry structure (must contain UTHash's hh field)
typedef struct fit_entry {
    void *key;                    // Hash key (function address)
    fit_bounds_t *bounds_data;    // Memory bounds array
    size_t bounds_num;            // Number of bounds

    uint8_t *syscalls;            // System call bitmap
    size_t syscalls_size;         // System call bitmap size (in bytes)

    uint8_t *maincalls;           // Main call bitmap
    size_t maincalls_size;        // Main call bitmap size (in bytes)

    void (*argbound_alloc)(va_list); // Callback function for argument bound allocation
    void (*argbound_free)(void);     // Callback function for argument bound deallocation

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

#endif // FIT_H