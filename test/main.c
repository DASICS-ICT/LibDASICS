#include <stdio.h>
#include <stdlib.h>
#include <udirect.h>
#include <udasics.h>
#include <uattr.h>
#include <string.h>

static void foo(void) __attribute__ ((constructor));

void exit_function() {
	printf("\n[Finish] test dasics finished\n");
}

void foo(void)
{
    printf("[Constructor] I am a Constructor function\n");
}

const char * test_str = "RISCV";
const char * format_str = "[%s] Hello riscv world: %d!\n";
static char dst[256] = {0};

#pragma
ATTR_ULIB_TEXT int my_memcpy()
{
    // copy 
    memcpy(dst, test_str, strlen(test_str));

    return 0;
}

/*
 * call_lib_noarg - invoke lib_call() for a wrapper that takes no user args.
 *
 * Why this helper is needed:
 * - lib_call now has a type-safe prototype: lib_call(func, va_list args).
 * - Even when we have "no logical arguments", we still need a valid va_list
 *   object created by va_start/va_end.
 *
 * This helper creates an empty variadic frame and forwards the resulting
 * va_list to lib_call(), so legacy no-arg test code can stay simple while
 * still obeying the explicit va_list API contract.
 */
static uint64_t call_lib_noarg(void *func_name, ...)
{
    va_list args;
    va_start(args, func_name);
    uint64_t ret = lib_call(func_name, args);
    va_end(args);
    return ret;
}


int main(int argc, char *argv[]) {
    // Add exit function 
    atexit(exit_function);
    register_udasics(0);

    int size = strlen(test_str);

    add_redirect_item("memcpy"); // memcpy
    add_redirect_item("strlen"); // strlen
    open_redirect();
    register uint64_t sp asm("sp");

    int idx0 = (int)LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_V, &test_str, sizeof(test_str));
    int idx1 = (int)LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_V, test_str, size);
    int idx2 = (int)LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, dst, 256);
    int idx3 = (int)LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, sp - 0x1000, 0x1000);


    printf("[LOG]: call my_memcpy\n");
    call_lib_noarg(my_memcpy);

    dasics_libcfg_free(idx0);
    dasics_libcfg_free(idx1);
    dasics_libcfg_free(idx2);
    dasics_libcfg_free(idx3);


    close_redirect();
    delete_redirect_item("memcpy"); // memcpy
    delete_redirect_item("strlen"); // strlen

    printf("%s\n", dst);


    unregister_udasics();
    return 0;
}