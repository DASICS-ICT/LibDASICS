#ifndef _UDASICS_H_
#define _UDASICS_H_

#include <stdio.h>
#include <stdint.h>
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
    Umaincall_PRINT,
    Umaincall_UNKNOWN
} UmaincallTypes;

// DASICS open/close
void register_udasics(uint64_t funcptr);
void unregister_udasics(void);

// source but don't include 
struct umaincall;

// DASICS maincall
uint64_t dasics_umaincall_helper(struct umaincall * regs, ...);

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

// DASICS jump bounds configure
int32_t dasics_jumpcfg_alloc(uint64_t lo, uint64_t hi);
int32_t dasics_jumpcfg_free(int32_t idx);


// extern uint64_t umaincall_helper;
extern void dasics_ufault_entry(void);
extern uint64_t dasics_umaincall(UmaincallTypes type, ...);
extern void lib_call(void* func_name, ...);
extern void azone_call(void* func_name);

#define LIBCFG_ALLOC(flag, base, len) (dasics_libcfg_alloc(flag,((uint64_t)(base)),((uint64_t)(base)) + ((uint64_t)(len))));

#endif
