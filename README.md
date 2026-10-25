# NanhuV5-FDI static SPEC runtime

The `NanhuV5-FDI` branch provides the static SPEC cross-domain runtime in
[spec-static](spec-static/README.md). Its ABI, user-mode trap handling, observation
interface and supported lifecycle are documented there. The manual below
describes the retained dynamic LibDASICS implementation.

[TOC]



# `LibDASICS` 用户手册

该用户手册介绍了 `LibDASICS` `dasics` 运行库提供的接口和用户手册说明。

## `DASICS libbounds` 管理接口

`LibDASICS` 利用软件模拟无限多组界限寄存器，由软件在触发 `DASICS` 中断时进行换入换出，每组 `libbound` 由一个`32`位的有符号整数标识管理。用户可使用如下三个接口进行 `libbounds` 的管理。

### `dasics_libcfg_alloc`

```C
int32_t  dasics_libcfg_alloc(uint64_t cfg, uint64_t lo, uint64_t hi);
```

**函数功能：** 分配一组界限寄存器并返回对应的 `handler`。

**参数说明：**

- `cfg`：`libbound` 寄存器权限配置，包括可读(`DASICS_LIBCFG_R`)，可写(`DASICS_LIBCFG_W`)以及有效位(`DASICS_LIBCFG_V`)。
- `lo`：界限下界。
- `hi`：界限上界。

**返回值：** 返回值为大于或等于0的 `int` 类型，失败返回 `-1`。



### `dasics_libcfg_free`

```C
int32_t  dasics_libcfg_free(int32_t idx);
```

**函数功能：** 释放分配的一组 `libbound` 寄存器。

**参数说明：**

- `idx`：通过 `dasics_libcfg_alloc` 分配界限寄存器组得到的 `handler。`

**返回值：** 函数执行成功返回0 ，失败返回 -1，失败表示未查找到该 `handler` 对应的 `libbound` 寄存器信息。 

 

### `dasics_libcfg_get`

```C
int32_t  dasics_libcfg_get(int32_t idx);
```

**函数功能：** 获取分配的一组 `libbounds` 寄存器的配置 `cfg` 权限信息。

**参数说明：**

- `idx`：通过 `dasics_libcfg_alloc` 分配界限寄存器组得到的 `handler`。

**返回值：** 函数执行成功返回 `libbound` 对应的权限信息，失败返回 `-1`，失败表示未查找到该 `handler` 对应的 `libbound` 寄存器信息。

## `DASICS jmpbounds` 管理接口

`LibDASICS` 目前未使用软件模拟无限多个 `jmpbounds`，目前仅仅支持`4`组活跃区配置寄存器，并仿照 `libbounds`提供了两个接口函数进行管理

### `dasics_jumpcfg_alloc`

```C
int32_t dasics_jumpcfg_alloc(uint64_t lo, uint64_t hi);
```

**函数功能：** 活跃区分配。

**函数参数：** 

- `lo`：活跃区下界
- `hi`：活跃区上界

**返回值：** 分配成功返回对应的 `handler`，失败返回 `-1`



### `dasics_jumpcfg_free`

```C
int32_t dasics_jumpcfg_free(int32_t idx);
```

**函数功能：** 释放分配的一组 `jmpbounds` 寄存器

**函数参数：**

- `idx`：通过 `dasics_jmpcfg_alloc` 分配界限寄存器组得到的 `handler`。



## 系统调用检查接口

`LibDASICS` 在初始化时对非可信区的所有系统调用采用默认不检查的方式，默认放行所有的非可信区的系统调用。用户可通过 `LibDASICS` 提供的接口对某个非可信区的系统调用设置检查函数和错误处理函数。

### `register_syscall_check`

```C
int register_syscall_check(int sysno, ecall_check_handler check_handler, ecall_error_handler error_handler)
```

**函数功能：** 为标号为 `sysno` 的系统调用设置检查函数和错误处理函数。若用户传递的处理函数为空，则处理方式保持原有不变。

**函数参数：**

- `syno`：系统调用号
- `check_handler`：用户定义的系统调用检查函数
- `error_handler`：用户定义的错误检查函数

**返回值：** 成功返回0，失败返回-1。





## 动态库调用

在程序调用动态库函数时，会进入到主函数调用接口，主函数查找到目标函数后设置好相应的权限并跳转到目标地址，并在此过程中进行跨库调用的处理。



## 非可信库内存分配操作

对于第三方库进行的 `malloc` 和 `free` 操作，第三方库进行 `malloc` 调用时，由主函数调用截取并代理完成内存和 `libbound` 寄存器的分配以及权限的设置；第三方库进行 `free` 调用时由主函数调用截取并代理完成内存的 `free` 并释放相应的 `libbound` 寄存器。其余的内存处理函数类似。

 


## NanhuV5-FDI ISA and recovery contract

The shared C/assembly definitions are in `include/dasics_isa.h`. The static
SPEC build uses the same definitions without importing the private dynamic
loader protocol.

| Interface | NanhuV5-FDI behavior |
| --- | --- |
| Main configuration | U view `0x9e1`, mask `0x3e`; S view `0xbc0`, mask `0x3ff`. `0x9e0` is Sfetchctl. |
| Calls | FDICALL uses custom3 (`0x7b`); existing target-register operands are retained. |
| Bounds | Half-open ranges, rounded outward to 8-byte grains. Empty, reversed, and ranges above the lower Sv48 user limit are rejected. |
| Wide user range | `[0, 0x800000000000)`; this broad permission policy does not provide per-buffer isolation. |
| Bound cache | A miss can refill only a complete, permission-matched access from one software grant. An already resident grant is not retried indefinitely. |
| Registration lifetime | `register_udasics` saves the prior FDI entries and bound bank; `unregister_udasics` restores them. `free_all` also removes nonresident software handles. |

Default load, store, jump, and unsupported delegated faults are denied without
advancing the faulting PC or authorizing its target. Registered `resgister_*`
handlers remain available for explicit recovery. They return a nonnegative
value on handled recovery and update the saved context as needed. Return-state
changes belong in `regs->return_pc` and `regs->freezone_return_pc`; direct CSR
writes inside a handler are superseded by the common context restoration.

A valid ECALL is checked against the syscall table. Check result zero permits
the trusted restore-first proxy; other results invoke the error callback.
Rejected or out-of-range syscalls return an error and resume after that ECALL.
A malformed fault or non-ECALL instruction is denied without such advancement.
A custom ECALL handler requests a proxy explicitly using
`syscall_action = DASICS_TRAP_PROXY_SYSCALL`.

The leading 160 bytes of `ucontext_trap` and 104 bytes of `umaincall` retain
their field offsets. Their structures grow to include floating-point and
return-state context, so users that allocate or inspect them must rebuild.
The maincall-specific callback typedef is `umaincall_check_handler`, avoiding
a conflicting definition of the syscall policy's `ecall_check_handler`.

`dynamic_hook` and `lib_call` keep target stack arguments at the original `sp`.
`lib_call` retains its historical ABI with the target address in `a0`; it is
not a general variadic argument-shifting adapter. Its return preserves any
bound-cache refills performed by the callee. The process-local side frames
remain single-threaded and do not implement general nonlocal-unwind cleanup.

The legacy dynamic entry still requires its private Linux/glibc auxiliary-vector
and relocation protocol. ISA correction alone does not make that protocol
available in the current static SPEC Linux stack.
