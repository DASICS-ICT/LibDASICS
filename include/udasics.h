#ifndef _UDASICS_H_
#define _UDASICS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <utrap.h>
#include "ucsr.h"
#include "uattr.h"


#ifdef DASICS_NGINX_MODE1
/* CSR field definitions come from the current dasics_isa.h via ucsr.h. */
#define align8up(addr) 		 ((addr+0x7) & ~(0x7))
#define align8down(addr) 	 (addr & ~(0x7))
#else
/* Public bounds are half-open; allocation rounds them out to 8-byte grains. */
#define align8up(addr) (((uint64_t)(addr) + 7UL) & ~7UL)
#define align8down(addr) ((uint64_t)(addr) & ~7UL)
#endif

// TODO: Add UmaincallTypes
typedef enum {
    Umaincall_PRINT,
    Umaincall_UNKNOWN
} UmaincallTypes;

// source but don't include 
struct umaincall;

// DASICS open/close
void register_udasics(uint64_t funcptr);
void unregister_udasics(void);

// DASICS user fault handler register
void resgister_uecall_fault_handler(utrap_handler ecall_fault_handler);
void resgister_uload_fault_handler(utrap_handler load_fault_handler);
void resgister_ustore_fault_handler(utrap_handler store_fault_handler);
void resgister_ufetch_fault_handler(utrap_handler fetch_fault_handler);


// DASICS maincall
#ifdef DASICS_NGINX_MODE1
uint64_t dasics_umaincall_helper(UmaincallTypes type, ...);
#else
void dasics_umaincall_helper(struct umaincall * regs, ...);
#endif

// source but don't include 
struct ucontext_trap;

// DASICS ufault handler
#ifdef DASICS_NGINX_MODE1
void dasics_ufault_handler(struct ucontext_trap * regs);

extern uint64_t libcfg;
extern uint64_t jumpcfg;
extern int dlibcfg_handle_map[DASICS_LIBCFG_WIDTH];

// DASICS memory bounds configure
int32_t  dasics_libcfg_alloc(uint64_t cfg, uint64_t lo, uint64_t hi);
int32_t  dasics_libcfg_free(int32_t handle);
#else
void     dasics_ufault_handler(struct ucontext_trap * regs);

// DASICS memory bounds configure
int32_t  dasics_libcfg_alloc(uint64_t cfg, uint64_t lo, uint64_t hi);
int32_t  dasics_libcfg_free(int32_t idx);
#endif
uint32_t dasics_libcfg_get(int32_t idx);
int32_t  dasics_libcfg_free_all();
void dasics_print_cfg_register(int32_t idx);

// TODO
#ifdef DASICS_NGINX_MODE1
int32_t dasics_libcfg_active(int32_t* handle, int num);
int32_t dasics_libcfg_inactive(int32_t* handle, int num);

#else
int32_t dasics_libcfg_active(int32_t idx);
#endif

// DASICS jump bounds configure
int32_t dasics_jumpcfg_alloc(uint64_t lo, uint64_t hi);
int32_t dasics_jumpcfg_free(int32_t idx);
int32_t dasics_jumpcfg_active(int32_t idx);
#ifdef DASICS_NGINX_MODE1
int32_t dasics_jumpcfg_inactive(int32_t idx);
#else
extern void restore_main();
#endif


// extern uint64_t umaincall_helper;
extern void dasics_ufault_entry(void);
extern uint64_t dasics_umaincall(UmaincallTypes type, ...);
extern void lib_call(void* func_name, ...);
extern void azone_call(void* func_name);

#define LIBCFG_ALLOC(flag, base, len) (dasics_libcfg_alloc(flag,((uint64_t)(base)),((uint64_t)(base)) + ((uint64_t)(len))));

#ifdef __cplusplus
}
#endif

#endif
