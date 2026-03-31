#ifndef _UDASICS_H_
#define _UDASICS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <utrap.h>
#include "ucsr.h"
#include "uattr.h"


/* DASICS Lib cfg */
#define DASICS_LIBCFG_WIDTH 16
#define DASICS_LIBCFG_MASK  0xfUL
#define DASICS_LIBCFG_V     0x8UL
#define DASICS_LIBCFG_X     0x4UL
#define DASICS_LIBCFG_R     0x2UL
#define DASICS_LIBCFG_W     0x1UL

#define DASICS_JUMPCFG_WIDTH 	4
#define DASICS_JUMPCFG_MASK 	0xffffUL
#define DASICS_JUMPCFG_V    	0x1UL

#define align8up(addr) 		 ((addr+0x7) & ~(0x7)) 
#define align8down(addr) 	 (addr & ~(0x7))

typedef enum {
    Umaincall_PRINT,
    Umaincall_MALLOC,
    Umaincall_PGRANT,
    Umaincall_TRANS,
    Umaincall_FREE,
    Umaincall_UNKNOWN
} UmaincallTypes;

// source but don't include 
struct umaincall;

// DASICS open/close
void register_udasics(void);
void unregister_udasics(void);

// DASICS user fault handler register
void register_uecall_fault_handler(utrap_handler ecall_fault_handler);
void register_uload_fault_handler(utrap_handler load_fault_handler);
void register_ustore_fault_handler(utrap_handler store_fault_handler);
void register_ufetch_fault_handler(utrap_handler fetch_fault_handler);


// DASICS maincall
uint64_t dasics_umaincall_helper(UmaincallTypes type, ...);

// source but don't include 
struct ucontext_trap;

// DASICS ufault handler
void     dasics_ufault_handler(struct ucontext_trap * regs);

// DASICS memory bounds configure
int32_t  dasics_libcfg_alloc(uint64_t cfg, uint64_t lo, uint64_t hi);
int32_t  dasics_libcfg_free(int32_t idx);
uint32_t dasics_libcfg_get(int32_t idx);
int32_t  dasics_libcfg_free_all();
void dasics_print_cfg_register(int32_t idx);

// TODO
int32_t dasics_libcfg_active(int32_t idx);

// DASICS jump bounds configure
int32_t dasics_jumpcfg_alloc(uint64_t lo, uint64_t hi);
int32_t dasics_jumpcfg_free(int32_t idx);
int32_t dasics_jumpcfg_free_all(void);
int32_t dasics_jumpcfg_active(int32_t idx);

// extern uint64_t umaincall_helper;
extern void dasics_ufault_entry(void);
extern uint64_t dasics_umaincall(UmaincallTypes type, ...);
/*
 * lib_call
 * --------
 * Type-safe FIT transition call entry.
 *
 * Semantics:
 * - The second parameter is a va_list object captured by the caller.
 * - This API does NOT "expand" variadic arguments by itself.
 * - FIT's current design expects the callee to be a wrapper with signature
 *   like: int wrapper(va_list args), and the wrapper extracts parameters
 *   with va_arg().
 *
 * Note:
 * - Handling of "too many arguments" (beyond the current register-forwarding
 *   strategy) is intentionally left for future compiler-side optimization.
 * - This header intentionally exposes only one public entry for transition
 *   calls, to avoid mixing two call conventions over time.
 */
extern uint64_t lib_call(void *func_name, va_list args);
/*
 * lib_call_context - PLT dynamic call entry (raw register context).
 *
 * Unlike lib_call (which shifts a1->a0 for wrapper(va_list) convention),
 * this stub restores original a0-a7 from @saved_regs and jumps to @func
 * via dasicscall.jr.  The target receives its arguments unmodified.
 */
extern uint64_t lib_call_context(void *func, uint64_t *saved_regs);
extern void azone_call(void* func_name);

#define LIBCFG_ALLOC(flag, base, len) (dasics_libcfg_alloc(flag,((uint64_t)(base)),((uint64_t)(base)) + ((uint64_t)(len))));

#ifdef __cplusplus
}
#endif

#endif
