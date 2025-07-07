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


/* DASICS Lib cfg */
#define DASICS_LIBCFG_WIDTH 16
#define DASICS_LIBCFG_MASK  0xfUL
#define DASICS_LIBCFG_V     0x8UL
#define DASICS_LIBCFG_R     0x2UL
#define DASICS_LIBCFG_W     0x1UL

#define DASICS_JUMPCFG_WIDTH 	4
#define DASICS_JUMPCFG_MASK 	0xffffUL
#define DASICS_JUMPCFG_V    	0x1UL

#define align8up(addr) 		 ((addr+0x7) & ~(0x7)) 
#define align8down(addr) 	 (addr & ~(0x7))

// TODO: Add UmaincallTypes
typedef enum {
    Umaincall_NULL,
    Umaincall_PRINT,
    Umaincall_UNKNOWN
} UmaincallTypes;

// DASICS nested macros
#define TYPE_MEM_BOUND 0
#define TYPE_JMP_BOUND 1

#define BNDQUERY_DENY  0x0
#define BNDQUERY_RO    0x1
#define BNDQUERY_RW    0x2
#define BNDQUERY_EMPTY 0x3
#define BNDQUERY_MASK  0x3

// source but don't include 
struct umaincall;

// DASICS open/close
void register_udasics(uint64_t funcptr);
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
void dasics_ufault_handler(struct ucontext_trap * regs);

extern uint64_t libcfg;
extern uint64_t jumpcfg;

// DASICS memory bounds configure
int32_t  dasics_libcfg_alloc(uint64_t cfg, uint64_t lo, uint64_t hi);
int32_t  dasics_libcfg_free(int32_t handle);
uint32_t dasics_libcfg_get(int32_t idx);
int32_t  dasics_libcfg_free_all();
void dasics_print_cfg_register(int32_t idx);

// TODO
int32_t dasics_libcfg_active(int32_t* handle, int num);
int32_t dasics_libcfg_inactive(int32_t* handle, int num);


// DASICS jump bounds configure
int32_t dasics_jumpcfg_alloc(uint64_t lo, uint64_t hi);
int32_t dasics_jumpcfg_free(int32_t idx);
int32_t dasics_jumpcfg_get(int32_t idx);
void    dasics_jumpcfg_free_all();
int32_t dasics_jumpcfg_active(int32_t idx);
int32_t dasics_jumpcfg_inactive(int32_t idx);


// extern uint64_t umaincall_helper;
extern void dasics_ufault_entry(void);
extern uint64_t dasics_umaincall(UmaincallTypes type, ...);
extern uint64_t lib_call(void *func_name, ...);
extern void azone_call(void* func_name);

#define LIBCFG_ALLOC(flag, base, len) (dasics_libcfg_alloc(flag,((uint64_t)(base)),((uint64_t)(base)) + ((uint64_t)(len))));

extern int32_t ATTR_ULIB_CALLER_TEXT dasics_ulib_libcfg_alloc(uint64_t cfg, uint64_t lo ,uint64_t hi);
extern int32_t ATTR_ULIB_CALLER_TEXT dasics_ulib_libcfg_free(int32_t idx);
extern int32_t ATTR_ULIB_CALLER_TEXT dasics_ulib_libcfg_copy(int src_idx);
extern int32_t ATTR_ULIB_CALLER_TEXT dasics_ulib_mem_get(int32_t idx, uint64_t *lo, uint64_t *hi);
extern int32_t ATTR_ULIB_CALLER_TEXT dasics_ulib_jumpcfg_alloc(uint64_t lo, uint64_t hi);
extern int32_t ATTR_ULIB_CALLER_TEXT dasics_ulib_jumpcfg_free(int32_t idx);
extern int32_t ATTR_ULIB_CALLER_TEXT dasics_ulib_jump_get(int32_t idx, uint64_t *lo, uint64_t *hi);

extern uint64_t dasics_ulib_libcall(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6, void *func_name);
#define dasics_ulib_libcall_no_args(func_name) (dasics_ulib_libcall(0, 0, 0, 0, 0, 0, 0, func_name))

extern void dasics_ulib_copy_mem_bound(int bound_src, int bound_dst);
extern void dasics_ulib_copy_jmp_bound(int bound_src, int bound_dst);
extern uint64_t dasics_ulib_query_mem_bound(void);
extern uint64_t dasics_ulib_query_jmp_bound(void);

#define dasics_ulib_copy_bound(type,bound_src,bound_dst) ((type)?\
                                                    dasics_ulib_copy_jmp_bound(bound_src, bound_dst):\
                                                    dasics_ulib_copy_mem_bound(bound_src, bound_dst))
#define dasics_ulib_query_bound(type) ((type)?\
                                  dasics_ulib_query_jmp_bound():\
                                  dasics_ulib_query_mem_bound())

#ifdef __cplusplus
}
#endif

#endif
