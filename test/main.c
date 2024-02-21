#include <stdio.h>
#include <stdlib.h>
#include <udasics.h>
#include <uattr.h>

static void foo(void) __attribute__ ((constructor));

void exit_function() {
	printf("[Finish] test dasics finished\n");
}

void foo(void)
{
    printf("[Constructor] I am a Constructor function\n");
}

#pragma GCC push_options
#pragma GCC optimize("O0")
void ATTR_UFREEZONE_TEXT test_free()
{
    // printf("[Free]: I am a free zone!\n");
    return;
}

void ATTR_ULIB_TEXT test_branch()
{
    // asm volatile (
    //      "addi   sp, sp, -8\n\r"
    //      "sd     ra, 0(sp)\n\r"
    //      "li a0, 0\n\r"
    //      "li a1, 2\n\r"
    //      "bne a0, a1, 1f\n\r"
    //      "1:\n\r"
    //      "ld     ra, 0(sp)\n\r"
    //      "addi   sp, sp, 8\n\r"
    //      "ret\n\r");
    test_free();
    // lib_call(&test_free);
    // register uint64_t a0 asm("a0") = &test_free;
    // asm (".word	0x0005108b");

}

#pragma GCC pop_options

void fuck()
{
    test_branch();
}
#define PAGE_SIZE 0x1000

const char * test_str = "RISCV";
const char * format_str = "[%s] Hello riscv world: %d!\n";
extern uint64_t umaincall_helper;
int main(int argc, char *argv[]) {
    // atexit(exit_function);
    // Test for redirect
    // add_redirect_item("printf");
    // add_redirect_item("puts");
    
    // open_redirect();
    
    // volatile int secret_num = 0x8badf00d;
    // char name[64] = {0};
    // int id1 = dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_V , (uint64_t)test_str, (uint64_t)test_str + strlen(test_str));
    // int id2 = dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_V ,  (uint64_t)format_str, (uint64_t)format_str + strlen(format_str));
    
    extern uint64_t __ULIBFREEZONETEXT_BEGIN__;
    extern uint64_t __ULIBFREEZONETEXT_END__;
    register long sp asm("sp");
    // int id3 = dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_V | DASICS_LIBCFG_W, sp - PAGE_SIZE, sp);
    // fuck();
    // int id4 = dasics_jumpcfg_alloc(&__ULIBFREEZONETEXT_BEGIN__, &__ULIBFREEZONETEXT_END__);
    umaincall_helper = (uint64_t)dasics_umaincall_helper;
    dasics_umaincall(Umaincall_PRINT, format_str, "test", 2);
    // dasics_libcfg_free(id3);
    // dasics_jumpcfg_free(id4);
    // Test for redirect
    // printf(format_str, test_str, 45);
    // printf(format_str, test_str, 66);
    // printf(format_str, test_str, secret_num);
    
    // // my_printf("name: %lx, sp: %lx\n", name, sp);
    // read(0, name, 64);
    // // printf("Hello ");
    // int id4 = dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_V, (uint64_t)name, (uint64_t)name + 64);
    // printf(name);
    // // printf("! You'll never get my secret!\n");
    
    // close_redirect();

    // dasics_libcfg_free(id1);
    // dasics_libcfg_free(id2);
    // // dasics_libcfg_free(id3);
    // dasics_libcfg_free(id4);

    // delete_redirect_item("printf");
    // delete_redirect_item("puts");


    for (int i = 0; i < argc; i++)
    {
        printf("\t[argc %d]: %s\n", i + 1, argv[i]);
    }
    
    return 0;
}