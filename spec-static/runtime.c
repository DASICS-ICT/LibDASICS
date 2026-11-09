#include "runtime.h"
#include "linux/fdi_verify.h"

#include <fcntl.h>
#include <stddef.h>

#define READ_CSR(csr) ({ unsigned long value; \
    __asm__ volatile("csrr %0, " DASICS_STRINGIFY(csr) : "=r"(value)); value; })
#define WRITE_CSR(csr, value) \
    __asm__ volatile("csrw " DASICS_STRINGIFY(csr) ", %0" :: "rK"((unsigned long)(value)) : "memory")

_Static_assert(sizeof(struct fdi_spec_call_frame) == FDI_SPEC_CALL_FRAME_SIZE,
               "call frame assembly layout");
_Static_assert(sizeof(struct fdi_spec_trap_frame) == FDI_SPEC_TRAP_FRAME_SIZE,
               "trap frame assembly layout");
_Static_assert(offsetof(struct fdi_spec_trap_frame, fpr) == 256,
               "floating context assembly offset");
_Static_assert(offsetof(struct fdi_spec_trap_frame, epc) == 520,
               "exception PC assembly offset");

struct fdi_spec_call_frame fdi_spec_call_frames[FDI_SPEC_FRAME_LIMIT];
uint64_t fdi_spec_call_depth;
static uint64_t ecall_count;
static unsigned int initialized, finished;
static unsigned int cleanup_started;
static uint64_t abandoned_frames, cleanup_entry_depth;
static long observer_fd = -1;
static uint64_t observer_cookie, observer_pid;

/* Only the prefixed libc owns the exit registry. Records are reused after
 * dispatch, so DSO finalization does not consume a monotonically growing log. */
struct callback {
    void (*function)(void *);
    void *argument;
    unsigned int active;
};
static struct callback callbacks[4096];

extern int __real_umain___cxa_atexit(void (*)(void *), void *, void *);
extern void __real_umain___cxa_finalize(void *);
extern _Noreturn void __real_umain_exit(int status);
extern void udasics__dl_aux_init(void *auxv);
extern void udasics___libc_early_init(unsigned char initial);
extern void udasics___init_misc(int argc, char **argv, char **environment);
extern char **__environ, **__libc_argv;
extern int __libc_argc;
/* Only omnetpp supplies these application-owned allocation bodies. Its
 * untrusted standard-library callers reach them through two fixed services,
 * retaining the benchmark's MemManager rather than replacing its allocator. */
extern void *fdi_spec_omnetpp_operator_new(size_t) __attribute__((weak));
extern void fdi_spec_omnetpp_operator_delete(void *) __attribute__((weak));

/* Weak references leave C and Fortran links independent of libstdc++. The
 * builder explicitly retains the real entries needed by each C++ program. */
extern _Noreturn void __real_umain___cxa_throw(void *, void *, void (*)(void *))
    __attribute__((weak));
extern _Noreturn void __real_umain___cxa_rethrow(void) __attribute__((weak));
extern _Noreturn void __real_umain__Unwind_Resume(void *) __attribute__((weak));
extern void *__real_umain___cxa_begin_catch(void *) __attribute__((weak));
extern void __real_umain___cxa_end_catch(void) __attribute__((weak));

/* No libc, TLS, allocation, or floating operations are needed before _start.
 * The six-argument proxy always clears a6 for the existing control syscall. */
static long raw_syscall(long number, long p0, long p1, long p2,
                        long p3, long p4, long p5)
{
    register long a0 __asm__("a0") = p0;
    register long a1 __asm__("a1") = p1;
    register long a2 __asm__("a2") = p2;
    register long a3 __asm__("a3") = p3;
    register long a4 __asm__("a4") = p4;
    register long a5 __asm__("a5") = p5;
    register long a6 __asm__("a6") = 0;
    register long a7 __asm__("a7") = number;
    __asm__ volatile("ecall" : "+r"(a0)
                     : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                       "r"(a6), "r"(a7) : "memory");
    return a0;
}

static _Noreturn void raw_exit(long status)
{
    raw_syscall(94, status, 0, 0, 0, 0, 0);
    for (;;) __asm__ volatile("ebreak");
}

static int write_all(int fd, const char *data, size_t count)
{
    while (count) {
        long written = raw_syscall(64, fd, (long)data, count, 0, 0, 0);
        if (written == -4) continue;
        if (written <= 0 || (unsigned long)written > count) return -1;
        data += written;
        count -= written;
    }
    return 0;
}

struct record {
    char data[1024];
    size_t size;
};

static void text(struct record *record, const char *value)
{
    while (*value && record->size < sizeof(record->data) - 1)
        record->data[record->size++] = *value++;
}

static void number(struct record *record, uint64_t value, unsigned int base)
{
    char digits[32];
    unsigned int count = 0;
    do {
        digits[count++] = "0123456789abcdef"[value % base];
        value /= base;
    } while (value);
    while (count && record->size < sizeof(record->data) - 1)
        record->data[record->size++] = digits[--count];
}

static void field(struct record *record, const char *name, uint64_t value)
{
    text(record, name);
    number(record, value, 10);
}

static void emit(struct record *record)
{
    text(record, "\n");
    if (write_all(FDI_SPEC_METADATA_FD, record->data, record->size)) {
        const char message[] = "FDI_SPEC_RUNTIME metadata descriptor is unavailable\n";
        write_all(2, message, sizeof(message) - 1);
        raw_exit(127);
    }
}

static void clear_observation(struct fdi_verify_result *result)
{
    /* Volatile byte stores avoid an implicit memset before libc startup. */
    volatile unsigned char *bytes = (volatile unsigned char *)result;
    for (size_t i = 0; i < sizeof(*result); ++i) bytes[i] = 0;
    result->version = FDI_VERIFY_VERSION;
}

static void begin_observation(void)
{
    struct fdi_verify_result result;
    long original = raw_syscall(56, AT_FDCWD, (long)"/dev/fdi-verify",
                                O_RDWR | O_CLOEXEC, 0, 0, 0);
    if (original < 0) fdi_spec_panic(16);
    /* Move the descriptor before BEGIN: closing any alias after BEGIN revokes
     * the device's observation window. Do not occupy the benchmark's low FDs. */
    observer_fd = raw_syscall(25, original, F_DUPFD_CLOEXEC,
                              FDI_SPEC_METADATA_FD + 1, 0, 0, 0);
    long closed = raw_syscall(57, original, 0, 0, 0, 0, 0);
    if (observer_fd < 0 || closed) fdi_spec_panic(16);
    clear_observation(&result);
    if (raw_syscall(29, observer_fd, FDI_VERIFY_BEGIN, (long)&result, 0, 0, 0) ||
        result.version != FDI_VERIFY_VERSION || result.flags ||
        !result.cookie || result.active != 1 || result.illegal_count ||
        result.pid != (uint64_t)raw_syscall(172, 0, 0, 0, 0, 0, 0))
        fdi_spec_panic(16);
    observer_cookie = result.cookie;
    observer_pid = result.pid;
}

static void end_observation(void)
{
    struct fdi_verify_result result;
    long status, closed;
    clear_observation(&result);
    status = raw_syscall(29, observer_fd, FDI_VERIFY_END, (long)&result, 0, 0, 0);
    closed = raw_syscall(57, observer_fd, 0, 0, 0, 0, 0);
    observer_fd = -1;
    if (status || closed || result.version != FDI_VERIFY_VERSION ||
        result.flags || result.cookie != observer_cookie ||
        result.pid != observer_pid || result.active ||
        (result.senvcfg & FDI_VERIFY_ENVCFG_MASK) != result.saved_envcfg ||
        (result.shadow_envcfg & FDI_VERIFY_ENVCFG_MASK) != result.saved_envcfg)
        fdi_spec_panic(16);
    if (result.illegal_count) {
        struct record record;
        record.size = 0;
        text(&record, "FDI_SPEC_ILLEGAL version=1");
        field(&record, " count=", result.illegal_count);
        field(&record, " cause=", result.cause);
        text(&record, " pc=0x"); number(&record, result.epc, 16);
        text(&record, " tval=0x"); number(&record, result.tval, 16);
        emit(&record);
        fdi_spec_panic(17);
    }
}

_Noreturn void fdi_spec_panic(uint64_t code)
{
    struct record record;
    record.size = 0;
    text(&record, "FDI_SPEC_RUNTIME version=1 event=fatal");
    field(&record, " code=", code);
    field(&record, " cause=", READ_CSR(CSR_UCAUSE));
    text(&record, " pc=0x"); number(&record, READ_CSR(CSR_UEPC), 16);
    text(&record, " tval=0x"); number(&record, READ_CSR(CSR_UTVAL), 16);
    field(&record, " reason=", READ_CSR(CSR_DFREASON));
    field(&record, " depth=", fdi_spec_call_depth);
    text(&record, "\n");
    write_all(FDI_SPEC_METADATA_FD, record.data, record.size);
    write_all(2, record.data, record.size);
    raw_exit(127);
}

struct eh_state {
    uint64_t libcfg, return_pc, freezone_return_pc;
};

static _Noreturn void eh_reject(unsigned long operation, const char *reason)
{
    struct record record;
    record.size = 0;
    text(&record, "FDI_SPEC_RUNTIME version=1 event=fatal code=19 error=unsupported_exception_transfer reason=");
    text(&record, reason);
    field(&record, " operation=", operation);
    field(&record, " depth=", fdi_spec_call_depth);
    text(&record, " libcfg=0x"); number(&record, READ_CSR(CSR_DLCFG), 16);
    text(&record, " return_pc=0x"); number(&record, READ_CSR(CSR_DRETURNPC), 16);
    text(&record, " freezone_return_pc=0x"); number(&record, READ_CSR(CSR_DFZRETURN), 16);
    emit(&record);
    raw_exit(127);
}

static struct eh_state eh_enter(unsigned long operation)
{
    struct eh_state state;
    if (!initialized || finished || fdi_spec_call_depth)
        eh_reject(operation, "active_wrapper_or_invalid_lifecycle");
    state.libcfg = READ_CSR(CSR_DLCFG);
    state.return_pc = READ_CSR(CSR_DRETURNPC);
    state.freezone_return_pc = READ_CSR(CSR_DFZRETURN);
    if (state.libcfg)
        eh_reject(operation, "active_library_permissions");
    return state;
}

static void eh_leave(unsigned long operation, struct eh_state saved)
{
    struct eh_state current = eh_enter(operation);
    if (current.libcfg != saved.libcfg || current.return_pc != saved.return_pc ||
        current.freezone_return_pc != saved.freezone_return_pc)
        eh_reject(operation, "return_state_changed");
}

/* These entries only guard trusted application EH. They never retire side
 * frames, rewrite permissions, or authorize an arbitrary untrusted landing pad. */
_Noreturn void __wrap_umain___cxa_throw(void *object, void *type,
                                      void (*destructor)(void *))
{
    eh_enter(FDI_SPEC_EH_THROW);
    if (destructor && ((uintptr_t)destructor < (uintptr_t)__fdi_trusted_start ||
                       (uintptr_t)destructor >= (uintptr_t)__fdi_trusted_end))
        eh_reject(FDI_SPEC_EH_THROW, "untrusted_exception_destructor");
    if (!__real_umain___cxa_throw)
        eh_reject(FDI_SPEC_EH_THROW, "missing_trusted_runtime");
    __real_umain___cxa_throw(object, type, destructor);
}

_Noreturn void __wrap_umain___cxa_rethrow(void)
{
    eh_enter(FDI_SPEC_EH_RETHROW);
    if (!__real_umain___cxa_rethrow)
        eh_reject(FDI_SPEC_EH_RETHROW, "missing_trusted_runtime");
    __real_umain___cxa_rethrow();
}

_Noreturn void __wrap_umain__Unwind_Resume(void *exception)
{
    eh_enter(FDI_SPEC_EH_RESUME);
    if (!__real_umain__Unwind_Resume)
        eh_reject(FDI_SPEC_EH_RESUME, "missing_trusted_runtime");
    __real_umain__Unwind_Resume(exception);
}

void *__wrap_umain___cxa_begin_catch(void *exception)
{
    struct eh_state state = eh_enter(FDI_SPEC_EH_BEGIN_CATCH);
    void *object;
    if (!__real_umain___cxa_begin_catch)
        eh_reject(FDI_SPEC_EH_BEGIN_CATCH, "missing_trusted_runtime");
    object = __real_umain___cxa_begin_catch(exception);
    eh_leave(FDI_SPEC_EH_BEGIN_CATCH, state);
    return object;
}

void __wrap_umain___cxa_end_catch(void)
{
    struct eh_state state = eh_enter(FDI_SPEC_EH_END_CATCH);
    if (!__real_umain___cxa_end_catch)
        eh_reject(FDI_SPEC_EH_END_CATCH, "missing_trusted_runtime");
    __real_umain___cxa_end_catch();
    eh_leave(FDI_SPEC_EH_END_CATCH, state);
}

void fdi_spec_initialize(void)
{
    struct record record;
    uintptr_t jump_lo = (uintptr_t)__fdi_untrusted_start & ~7UL;
    uintptr_t jump_hi = ((uintptr_t)__fdi_untrusted_end + 7UL) & ~7UL;
    /* The current Linux stack and default mmap window use the lower Sv48
     * address range. Include that window in this deliberately broad policy. */
    const unsigned long data_hi = DASICS_USER_LIMIT;
    record.size = 0;
    long cfg = raw_syscall(306, 0x4441534943534c58L, 0x515259, 0, 0, 0, 0);
    if (cfg != 2 || initialized || !fdi_spec_wrapper_count ||
        (uintptr_t)__fdi_untrusted_end <= (uintptr_t)__fdi_untrusted_start ||
        (uintptr_t)__fdi_trusted_end > jump_lo)
        fdi_spec_panic(1);

    /* BEGIN(flags=0) observes real cause 2 events without enabling CBO or
     * changing permissions. No ARM filter is needed for whole-process counts. */
    begin_observation();
    WRITE_CSR(CSR_UTVEC, fdi_spec_trap_entry);
    WRITE_CSR(CSR_DMAINCALL, fdi_spec_maincall);
    WRITE_CSR(CSR_USCRATCH, 0);
    WRITE_CSR(CSR_UTIMER, 0);
    WRITE_CSR(CSR_DLBOUND0LO, 0);
    WRITE_CSR(CSR_DLBOUND0HI, data_hi);
    WRITE_CSR(CSR_DLCFG, 0);
    /* Branches and internal library calls stay within the untrusted text.
     * MainCall and ReturnPC authorize the separate trusted transitions. */
    WRITE_CSR(CSR_DJBOUND0LO, jump_lo);
    WRITE_CSR(CSR_DJBOUND0HI, jump_hi);
    WRITE_CSR(CSR_DJCFG, 1);
    if (READ_CSR(CSR_UTVEC) != (uintptr_t)fdi_spec_trap_entry ||
        READ_CSR(CSR_DMAINCALL) != (uintptr_t)fdi_spec_maincall ||
        READ_CSR(CSR_DLBOUND0LO) != 0 || READ_CSR(CSR_DLBOUND0HI) != data_hi ||
        READ_CSR(CSR_DLCFG) != 0 || READ_CSR(CSR_DJBOUND0LO) != jump_lo ||
        READ_CSR(CSR_DJBOUND0HI) != jump_hi || READ_CSR(CSR_DJCFG) != 1)
        fdi_spec_panic(5);
    initialized = 1;
    text(&record, "FDI_SPEC_RUNTIME version=1 event=init mode=");
    text(&record, FDI_SPEC_DIAGNOSTIC ? "diagnostic" : "timing");
    text(&record, " user_trap=1 data_policy=wide-rw observer=active observer_flags=0 illegal=0");
    field(&record, " wrappers=", fdi_spec_wrapper_count);
    emit(&record);
}

void fdi_spec_trap_dispatch(struct fdi_spec_trap_frame *frame)
{
    unsigned long cause = READ_CSR(CSR_UCAUSE);
    unsigned long reason = READ_CSR(CSR_DFREASON);
    unsigned long pc = frame->epc;
    unsigned long syscall = frame->gpr[16];
    const volatile unsigned char *instruction = (const void *)pc;

    if (!initialized || finished || cause != 24 || reason != 1 ||
        pc < (uintptr_t)__fdi_untrusted_start ||
        pc > (uintptr_t)__fdi_untrusted_end - 4 ||
        instruction[0] != 0x73 || instruction[1] || instruction[2] || instruction[3])
        fdi_spec_panic(2);

    /* These interfaces resume on a new stack, signal frame, or executable.
     * A normal returning proxy cannot preserve their ABI. */
    if (syscall == 139 || syscall == 220 || syscall == 221 ||
        syscall == 281 || syscall == 435)
        fdi_spec_panic(3);
    ecall_count++;
    if (syscall == 93 || syscall == 94)
        fdi_spec_direct_exit((long)frame->gpr[9]);

    frame->gpr[9] = raw_syscall(syscall, frame->gpr[9], frame->gpr[10],
                                frame->gpr[11], frame->gpr[12],
                                frame->gpr[13], frame->gpr[14]);
    frame->epc += 4;
}

void fdi_spec_finish(void)
{
    struct record record;
    uint64_t calls = 0;
    record.size = 0;
    if (!initialized || finished || !cleanup_started || fdi_spec_call_depth)
        fdi_spec_panic(4);
    end_observation();
#if FDI_SPEC_DIAGNOSTIC
    for (uint64_t i = 0; i < fdi_spec_wrapper_count; ++i)
        calls += fdi_spec_wrapper_hits[i];
#endif
    text(&record, "FDI_SPEC_RUNTIME version=1 event=exit status=PASS depth=0 cleanup=complete observer=closed illegal=0");
    field(&record, " abandoned_frames=", abandoned_frames);
    field(&record, " ecall=", ecall_count);
    text(&record, " calls=");
    if (FDI_SPEC_DIAGNOSTIC) number(&record, calls, 10);
    else text(&record, "unavailable");
    emit(&record);
#if FDI_SPEC_DIAGNOSTIC
    for (uint64_t i = 0; i < fdi_spec_wrapper_count; ++i) {
        if (!fdi_spec_wrapper_hits[i]) continue;
        record.size = 0;
        text(&record, "FDI_SPEC_HIT version=1");
        field(&record, " id=", i);
        field(&record, " count=", fdi_spec_wrapper_hits[i]);
        text(&record, " name="); text(&record, fdi_spec_wrapper_names[i]);
        emit(&record);
    }
#endif
    finished = 1;
}

void fdi_spec_loader_initialize(const uintptr_t *initial_stack)
{
    uintptr_t argc;
    char **environment;
    char **auxiliary;
    if (!initialized || !initial_stack || fdi_spec_call_depth)
        fdi_spec_panic(7);
    argc = initial_stack[0];
    if (initial_stack[argc + 1]) fdi_spec_panic(7);
    environment = (char **)(initial_stack + argc + 2);
    auxiliary = environment;
    while (*auxiliary) ++auxiliary;
    /* This leaf initializer uses no TLS or libc services. Its auxv-derived
     * state includes RELRO data, so it must precede the normal CRT entry. */
    udasics__dl_aux_init(auxiliary + 1);
}

void fdi_spec_preinitialize(int argc, char **argv, char **environment)
{
    if (!initialized || !environment || fdi_spec_call_depth)
        fdi_spec_panic(7);
    __environ = environment;
    __libc_argc = argc;
    __libc_argv = argv;
    udasics___libc_early_init(1);
    udasics___init_misc(argc, argv, environment);
}

/* Static TLS is ready when preinit runs, while no library constructor has run.
 * This element must remain trusted and precede all generated ctor wrappers. */
__attribute__((used, section(".preinit_array.00000")))
static void (*const preinitialize)(int, char **, char **) = fdi_spec_preinitialize;

static int callback_domain(uintptr_t function)
{
    if (function >= (uintptr_t)__fdi_trusted_start &&
        function < (uintptr_t)__fdi_trusted_end) return 0;
    if (function >= (uintptr_t)__fdi_untrusted_start &&
        function < (uintptr_t)__fdi_untrusted_end) return 1;
    return -1;
}

static void callback_dispatch(void *argument)
{
    struct callback *record = argument;
    void (*function)(void *) = record->function;
    void *value = record->argument;
    int domain = callback_domain((uintptr_t)function);
    if (!record->active || domain < 0) fdi_spec_panic(8);
    record->active = 0;
    if (domain)
        fdi_spec_invoke_callback((uintptr_t)function, (uintptr_t)value);
    else function(value);
}

int fdi_spec_cxa_atexit(void (*function)(void *), void *argument, void *dso)
{
    if (!initialized || finished || callback_domain((uintptr_t)function) < 0)
        fdi_spec_panic(9);
    for (unsigned int i = 0; i < sizeof(callbacks) / sizeof(callbacks[0]); ++i) {
        struct callback *record = &callbacks[i];
        int result;
        if (record->active) continue;
        record->function = function;
        record->argument = argument;
        record->active = 1;
        result = __real_umain___cxa_atexit(callback_dispatch, record, dso);
        if (result) record->active = 0;
        return result;
    }
    return -1;
}

void fdi_spec_cxa_finalize(void *dso)
{
    if (!initialized || finished) fdi_spec_panic(10);
    __real_umain___cxa_finalize(dso);
}

void fdi_spec_main_return(void)
{
    /* A normal return cannot abandon a trampoline frame. Only an explicit
     * noreturn exit service may discard live frames at the terminal exit. */
    if (!initialized || finished || fdi_spec_call_depth) fdi_spec_panic(11);
}

_Noreturn void fdi_spec_process_exit(long status)
{
    if (!initialized || finished || cleanup_started) fdi_spec_panic(12);
    cleanup_entry_depth = fdi_spec_call_depth;
    cleanup_started = 1;
    __real_umain_exit((int)status);
}

_Noreturn void fdi_spec_terminal_exit(long status)
{
    if (!cleanup_started || finished || fdi_spec_call_depth != cleanup_entry_depth)
        fdi_spec_panic(13);
    for (unsigned int i = 0; i < sizeof(callbacks) / sizeof(callbacks[0]); ++i)
        if (callbacks[i].active) fdi_spec_panic(14);
    /* Only the trusted libc exit.o tail references this symbol. Its callback
     * registry and both libc cleanup linker-set entries have now completed.
     * No abandoned wrapper can return after this process-wide termination. */
    abandoned_frames = fdi_spec_call_depth;
    fdi_spec_call_depth = 0;
    fdi_spec_finish();
    raw_exit(status);
}

_Noreturn void fdi_spec_direct_exit(long status)
{
    struct record record;
    record.size = 0;
    text(&record, "FDI_SPEC_RUNTIME version=1 event=exit status=DIRECT_EXIT cleanup=skipped");
    field(&record, " depth=", fdi_spec_call_depth);
    field(&record, " ecall=", ecall_count);
    text(&record, " calls=unavailable");
    emit(&record);
    raw_exit(status);
}

long fdi_spec_service(long p0, long p1, long p2, unsigned long service)
{
    switch (service) {
    case FDI_SPEC_SERVICE_ATEXIT:
        return fdi_spec_cxa_atexit((void (*)(void *))p0, (void *)p1, (void *)p2);
    case FDI_SPEC_SERVICE_FINALIZE:
        fdi_spec_cxa_finalize((void *)p0);
        return 0;
    case FDI_SPEC_SERVICE_EXIT:
        fdi_spec_process_exit(p0);
    case FDI_SPEC_SERVICE_DIRECT_EXIT:
        fdi_spec_direct_exit(p0);
    case FDI_SPEC_SERVICE_OMNETPP_NEW:
        if (!initialized || finished || !fdi_spec_omnetpp_operator_new ||
            callback_domain((uintptr_t)fdi_spec_omnetpp_operator_new) != 0)
            fdi_spec_panic(18);
        return (long)fdi_spec_omnetpp_operator_new((size_t)p0);
    case FDI_SPEC_SERVICE_OMNETPP_DELETE:
        if (!initialized || finished || !fdi_spec_omnetpp_operator_delete ||
            callback_domain((uintptr_t)fdi_spec_omnetpp_operator_delete) != 0)
            fdi_spec_panic(18);
        fdi_spec_omnetpp_operator_delete((void *)p0);
        return 0;
    case FDI_SPEC_SERVICE_EH_UNSUPPORTED:
        eh_reject((unsigned long)p0, "untrusted_eh_entry");
    default:
        fdi_spec_panic(15);
    }
}
