#ifndef FDI_SPEC_RUNTIME_H
#define FDI_SPEC_RUNTIME_H

#include "dasics_isa.h"

#define FDI_SPEC_METADATA_FD 198
#define FDI_SPEC_FRAME_LIMIT 1024
#define FDI_SPEC_CALL_FRAME_SIZE 64
#define FDI_SPEC_TRAP_FRAME_SIZE 576
#define FDI_SPEC_SERVICE_ATEXIT 1
#define FDI_SPEC_SERVICE_FINALIZE 2
#define FDI_SPEC_SERVICE_EXIT 3
#define FDI_SPEC_SERVICE_DIRECT_EXIT 4
#define FDI_SPEC_SERVICE_OMNETPP_NEW 5
#define FDI_SPEC_SERVICE_OMNETPP_DELETE 6

#ifndef FDI_SPEC_DIAGNOSTIC
#define FDI_SPEC_DIAGNOSTIC 0
#endif

#ifndef __ASSEMBLER__
#include <stdint.h>

/* This runtime is single-threaded. Untrusted data access is deliberately broad;
 * the side stack is bookkeeping, not a protected shadow-stack security claim. */
struct fdi_spec_call_frame {
    uint64_t ra, return_pc, freezone_return_pc, s11;
    uint64_t sp, wrapper_id, libcfg, reserved;
};

struct fdi_spec_trap_frame {
    uint64_t gpr[31];
    uint64_t integer_padding;
    uint64_t fpr[32];
    uint64_t fcsr, epc, scratch, return_pc, freezone_return_pc, status;
    uint64_t reserved[2];
};

extern struct fdi_spec_call_frame fdi_spec_call_frames[FDI_SPEC_FRAME_LIMIT];
extern uint64_t fdi_spec_call_depth;
extern const uint64_t fdi_spec_wrapper_count;
extern uint64_t fdi_spec_wrapper_hits[];
extern const char *const fdi_spec_wrapper_names[];
extern const uint64_t fdi_spec_callback_id;
extern char __fdi_untrusted_start[], __fdi_untrusted_end[];
extern char __fdi_trusted_start[], __fdi_trusted_end[];

void fdi_spec_initialize(void);
void fdi_spec_loader_initialize(const uintptr_t *initial_stack);
void fdi_spec_trap_dispatch(struct fdi_spec_trap_frame *frame);
void fdi_spec_finish(void);
void fdi_spec_call(void);
void fdi_spec_trap_entry(void);
void fdi_spec_maincall(void);
void fdi_spec_invoke_callback(uintptr_t function, uintptr_t argument);
void fdi_spec_main_return(void);
void fdi_spec_preinitialize(int argc, char **argv, char **environment);
int fdi_spec_cxa_atexit(void (*function)(void *), void *argument, void *dso);
void fdi_spec_cxa_finalize(void *dso);
long fdi_spec_service(long p0, long p1, long p2, unsigned long service);
_Noreturn void fdi_spec_panic(uint64_t code);

/* The exit bridge must finish both libc cleanup paths before calling finish.
 * Raw exit/exit_group from untrusted code must enter this bridge, not return
 * through a syscall-proxy frame that is no longer live. */
_Noreturn void fdi_spec_process_exit(long status);
_Noreturn void fdi_spec_terminal_exit(long status);
_Noreturn void fdi_spec_direct_exit(long status);
#endif

#endif
