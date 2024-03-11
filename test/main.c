#include <stdio.h>
#include <stdlib.h>
#include <udirect.h>
#include <udasics.h>
#include <malloc-free.h>

static void foo(void) __attribute__ ((constructor));

void exit_function() {
	printf("\n[Finish] test dasics finished\n");
}

void foo(void)
{
    printf("[Constructor] I am a Constructor function\n\n");
}

const char * test_str = "RISCV";
const char * format_str = "[%s] Hello riscv world: %d!\n";
int main(int argc, char *argv[]) {
    // Add exit function 
    
    atexit(exit_function);
    register_udasics(0);

    char * tempelate = "I am messgae which contains a very long message: %d";

    char buff[256];

    dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_W | DASICS_LIBCFG_V, \
                    (uint64_t)buff, \
                    (uint64_t)buff +256);
    int i;
    for (i = 0; i < 8; i++)
    {
        sprintf(buff, tempelate, i);
        call_and_record(MALLOC, buff);
    }


    // Free one
    call_and_record(FREE, NULL);

    // Realloc one
    call_and_record(REALLOC, NULL);

    // Free one
    call_and_record(FREE, NULL);


    // sprintf(buff, tempelate, i);
    call_and_record(MALLOC, buff);

    call_and_record(FREE, NULL);

    unregister_udasics();
    return 0;
}