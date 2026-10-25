# Static SPEC runtime

This directory provides the user-mode runtime for statically linked SPEC
CPU2006 programs on the current NanhuV5 FDI Linux/OpenSBI stack. The integration
repository's `scr/spec2006/build_fdi_apps.py` supplies the rewritten application
objects, two library code domains, wrapper tables, and linker layout.

The workload measures cross-domain function calls and their overhead. A library
call receives read/write permission over `[0, 1 << 47)`, covering the current
Linux lower Sv48 stack and default mmap window. The historical Sv39 limit
`1 << 38` excludes that stack. This is not a per-argument or per-buffer isolation
policy.

## Entry and call ABI

| Entry | Contract |
| --- | --- |
| `_fdi_spec_start` | Runs before the normal CRT entry. Initializes `gp`, checks the existing Linux FDI configuration query, installs and reads back the U-trap and maincall entries and bounds. No libc or TLS dependency. |
| `fdi_spec_loader_initialize` | Parses auxv from the original kernel stack and calls the untrusted libc's leaf `_dl_aux_init` before the normal CRT seals RELRO. This initializer has no TLS dependency. |
| `fdi_spec_preinitialize` | Runs first in `.preinit_array`, after static TLS exists and before library constructors. Initializes the untrusted libc's environment, early TLS state, and program name. |
| `fdi_spec_call` | Receives the untrusted target in `t0` and wrapper ID in `t1`. Preserves integer/floating arguments, stack arguments, `tp`, integer/floating results, and callee-saved state. |
| `fdi_spec_maincall` | Provides only the explicitly enumerated registration, finalization, exit, and omnetpp allocator services. It does not recover arbitrary jump faults. |
| `umain_main` | Calls the application `main` and checks that a normal return leaves no active wrapper frames. |

Generated wrappers must use `lla t2, fdi_spec_call; jr t2` after setting `t0`
and `t1`: the ordinary `tail` pseudo-instruction uses `t1` as a scratch register.

The call trampoline leaves `sp` unchanged while entering the callee, preserving
stack-passed arguments and 16-byte alignment. A bounded side stack saves the
caller return address, `s11`, DASICS return CSRs, and enclosing data permissions.
Each call enables the wide read/write bound and restores the previous
configuration on return. The jump bound covers only `.ulibtext`, including its
alignment padding; trusted transitions use the dedicated architectural entries.
The side stack is runtime bookkeeping, not a protected shadow-stack guarantee.

The generator provides `__fdi_trusted_start/end`, `__fdi_untrusted_start/end`,
`fdi_spec_wrapper_count`, `fdi_spec_wrapper_hits`, `fdi_spec_wrapper_names`, and
`fdi_spec_callback_id`. All runtime code except the explicit untrusted service
stubs must remain inside the trusted `.text` region.

For omnetpp, the generator preserves the application's existing scalar
`operator new` and `operator delete` bodies under the private symbols
`fdi_spec_omnetpp_operator_new` and `fdi_spec_omnetpp_operator_delete`.
Application references still call those bodies directly. Standard-library
references to `_Znwm` and `_ZdlPv` enter untrusted stubs and use fixed maincall
services 5 and 6, respectively. The services preserve the original size/pointer
signatures and MemManager behavior. Other programs do not provide those weak
symbols; an attempted service call then fails instead of dereferencing zero.

## User-mode traps

The trap entry saves all integer registers, all 32 floating-point registers,
`fcsr`, and the relevant return CSRs. Only cause 24 with ECALL reason 1, a PC
inside `.ulibtext`, and an actual ECALL instruction is proxied. The proxy executes
a real system call from trusted user text, returns its raw result in `a0`, and
resumes after the original ECALL with `uret`.

Unexpected delegated load, store, jump, timer, or other traps terminate with a
fatal record. Untrusted `clone`, `clone3`, `execve`, `execveat`, and `rt_sigreturn`
are rejected: their context-changing ABI cannot return through this proxy frame.
This runtime assumes a single user thread.

The current Linux illegal-instruction handler skips instructions inside
`.ulibtext`, so cause 2 events do not reach this U-trap handler. The runtime also
opens the existing `/dev/fdi-verify` observer with `BEGIN(flags=0)`. No ARM filter
is installed: the observer records every user-origin cause 2 for the current
process. At terminal exit, `END` must preserve its version, cookie, process ID,
and configuration identity and report zero illegal instructions. A skipped
illegal instruction therefore rejects the run even if its output happens to
match. The observer does not enable CBO or change permissions.

The existing `fdi_verify` module must be loaded, and the process requires its
existing `CAP_SYS_RAWIO` access. The build uses the current Linux
`include/uapi/linux/fdi_verify.h`; it does not introduce a kernel API. The
observer descriptor is moved above 198 before opening the observation window.
Closing or replacing it during a benchmark invalidates the final check.

## Constructors and process exit

The build supplies trusted wrappers for untrusted constructor/destructor and
`__libc_atexit` linker-set entries. It must retain this runtime's preinit entry
first and share the process-break cache between the two libc copies.
The initial stack-end pointer is also a single shared slot initialized by the
trusted CRT. The runtime does not rewrite that RELRO slot from preinit.

Both copies' `__cxa_atexit` and `__cxa_finalize` use one trusted libc registry.
Registered callbacks are dispatched according to their actual text domain.
Normal `exit`, including a Fortran STOP path, enters the trusted exit routine;
that routine completes registered callbacks and both libc I/O cleanup entries.

Only the trusted libc `exit.o` member's final `_exit` reference is redirected to
`fdi_spec_terminal_exit`. This is the sole full-cleanup success point. Cleanup
must leave the wrapper depth unchanged from entry. An explicit nonreturning exit
can then discard its remaining frames, recording their count. A normal return
from `main` must already have depth zero.

Direct `_exit` retains its direct-termination behavior and records
`status=DIRECT_EXIT cleanup=skipped`; the complete-cleanup regression oracle does
not accept it as a normal successful run. Recursive exit during cleanup is
currently rejected. There are at most 1,024 active wrapper frames and 4,096
simultaneously registered runtime callback records; completed callback records
are reusable.

## Metadata and diagnostic builds

The launcher must open descriptor **198** as a separate per-invocation runtime
log and preserve it across `exec`. Missing or unwritable metadata output is an
error. SPEC stdout and stderr remain reserved for the benchmark's own output;
fatal diagnostics are additionally written to stderr.

A timing build writes these records without per-call function counters:

```text
FDI_SPEC_RUNTIME version=1 event=init mode=timing user_trap=1 data_policy=wide-rw observer=active observer_flags=0 illegal=0 wrappers=N
FDI_SPEC_RUNTIME version=1 event=exit status=PASS depth=0 cleanup=complete observer=closed illegal=0 abandoned_frames=N ecall=N calls=unavailable
```

`FDI_SPEC_DIAGNOSTIC=1` enables wrapper hit counters and emits `FDI_SPEC_HIT`
records at termination. Diagnostic builds provide observed function coverage;
they are not the formal timing binaries. Initialization and exit records must
be checked together with process status and the original SPEC output oracle.

The assembly includes CFI descriptions for the side-frame return address and
`s11`. It does not implement general cleanup for exceptions or `longjmp` that
cross an active wrapper. CFI alone is not evidence of full nonlocal-unwind
support. Initialization, callbacks, and normal benchmark behavior still require
target validation before the runtime is considered usable for a test batch.
