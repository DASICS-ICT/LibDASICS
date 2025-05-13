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


// int main(int argc, char *argv[]) {
//     // Add exit function 
//     atexit(exit_function);
//     register_udasics(0);

//     int size = strlen(test_str);

//     add_redirect_item("memcpy"); // memcpy
//     add_redirect_item("strlen"); // strlen
//     open_redirect();
//     register uint64_t sp asm("sp");

//     int idx0 = (int)LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_V, &test_str, sizeof(test_str));
//     int idx1 = (int)LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_V, test_str, size);
//     int idx2 = (int)LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, dst, 256);
//     int idx3 = (int)LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, sp - 0x1000, 0x1000);


//     printf("[LOG]: call my_memcpy\n");
//     lib_call(my_memcpy);

//     dasics_libcfg_free(idx0);
//     dasics_libcfg_free(idx1);
//     dasics_libcfg_free(idx2);
//     dasics_libcfg_free(idx3);


//     close_redirect();
//     delete_redirect_item("memcpy"); // memcpy
//     delete_redirect_item("strlen"); // strlen

//     printf("%s\n", dst);


//     unregister_udasics();
//     return 0;
// }


int main()
{
    atexit(exit_function);

    int32_t idx[8] = {0};

    for (int i = 0; i < 8; i++)
    {
        idx[i] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, i * 0x1000, (i + 1) * 0x1000);
    }

    int32_t idx_copy[8] = {0};
    dasics_libcfg_inactive(idx, 8);
    for (int i = 0; i < 8; i++)
    {
        idx_copy[i] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, i * 0x1000, (i + 1) * 0x1000);
    }

    dasics_libcfg_active(idx, 8);


    int32_t idx2[8] = {0};
    for (int i = 0; i < 8; i++)
    {
        idx2[i] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, i * 0x1000, (i + 1) * 0x1000);
    }

    dasics_libcfg_active(idx, 8);

    printf("[LOG]: test dasics_libcfg_active\n");
    return 0;
}