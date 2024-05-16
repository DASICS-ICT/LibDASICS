#include <stdio.h>
#include <stdlib.h>
#include <udirect.h>
#include <udasics.h>
#include <uattr.h>
#include <uwrapper.h>
// #define PRINT_DEBUG

#define readcycle csr_read(cycle)

static long invoke_syscall(long sysno, long arg0, long arg1, long arg2,
                           long arg3, long arg4, long arg5, long arg6)
{
    register long a7 asm("a7") = sysno;
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a2 asm("a2") = arg2;
    register long a3 asm("a3") = arg3;
    register long a4 asm("a4") = arg4;
    register long a5 asm("a5") = arg5;
    register long a6 asm("a6") = arg6;

    asm volatile("ecall"                      \
                 : "+r"(a0)                   \
                 : "r"(a1), "r"(a2), "r"(a3), \
                   "r"(a4), "r"(a5), "r"(a6), \
                   "r"(a7)                    \
                 : "memory");

    return a0;
}

#define align8up(addr) 		 ((addr+0x7) & ~(0x7)) 
#define align8down(addr) 	 (addr & ~(0x7))


static char ATTR_ULIB1_DATA secret[128] = "[ULIB1]: It's the secret!\n";
static char ATTR_ULIB1_DATA ulib1_readonly[128] = "[ENTER ULIB1]: This is ro to ULIB1 and invisible to ULIB2!\n";
static char ATTR_ULIB1_DATA ulib1_rwbuffer[128] = "[ULIB1]: This is rw to ULIB1 and ro to ULIB2!\n";

static char ATTR_ULIB1_DATA main_prompt1[128] = "[UMAIN]: Ready to enter dasics_ulib1.\n";
static char ATTR_ULIB1_DATA main_prompt2[128] = "[UMAIN]: DasicsLibCfg0: 0x%lx\n";
static char ATTR_ULIB1_DATA main_prompt3[128] = "[UMAIN]: DasicsReturnPC: 0x%lx\n";
static char ATTR_ULIB1_DATA main_prompt4[128] = "[UMAIN]: DasicsFreeZoneReturnPC: 0x%lx\n";

static char ATTR_ULIB1_DATA ulib2_rwbuffer[128] = "[ENTER ULIB2]: This is rw to ULIB1 and ULIB2!\n";

void ATTR_ULIB1_TEXT dasics_ulib_nested(void);
void ATTR_ULIB1_TEXT dasics_ulib_maincall(void);
void ATTR_ULIB2_TEXT dasics_ulib2(void);

static void foo(void) __attribute__ ((constructor));

void exit_function() {
	printf("\n[Finish] test dasics finished\n");
}

void foo(void)
{
    printf("[Constructor] I am a Constructor function\n\n");
}

// #pragma GCC push_options
// #pragma GCC optimize("O0")
static void ATTR_ULIB1_TEXT dasics_ulib1_printf(uint64_t fmt){
    dasics_umaincall(Umaincall_PRINT,fmt,0,0);
}

static void ATTR_ULIB1_TEXT dasics_ulib1_printf_1(uint64_t fmt, uint64_t arg0){
    dasics_umaincall(Umaincall_PRINT,fmt,arg0,0);
}

static inline uint64_t ATTR_ULIB1_TEXT roundup8_ulib1(uint64_t x) {
    int roundup = (x & 7) ? 8 : 0;
    return (x & ~7UL) + roundup;
}

static void ATTR_ULIB2_TEXT dasics_ulib2_printf(uint64_t fmt){
    dasics_umaincall(Umaincall_PRINT,fmt,0,0);
}



void ATTR_ULIB2_TEXT dasics_ulib2(void){

#ifdef PRINT_DEBUG
    dasics_ulib2_printf((uint64_t) ulib2_rwbuffer); // That's ok
    dasics_ulib2_printf((uint64_t) ulib2_rwbuffer); // That's ok
#endif

}

void ATTR_ULIB1_TEXT dasics_ulib_nested(void) {
    // uint64_t dasicsNestedPreticks = dasics_umaincall(Umaincall_GET_TICK);
    // uint64_t dasicsNestedPreticks = dasics_umaincall(Umaincall_getclock);


    for (int i = 0; i < 100; i++)
    {
        uint64_t dasicsNestedPreticks = readcycle;

    #ifdef PRINT_DEBUG
        dasics_ulib1_printf((uint64_t) ulib1_readonly);
        dasics_ulib1_printf((uint64_t) ulib1_rwbuffer);
    #endif

        //SET ULIB2 ENVIRONMENT
        int idx0, idx1, idx2, idx3, idx4;
        idx0 = dasics_ulib_libcfg_alloc(DASICS_LIBCFG_V | DASICS_LIBCFG_R                  , (uint64_t)ulib1_rwbuffer, (uint64_t)(ulib1_rwbuffer + 128));
        idx1 = dasics_ulib_libcfg_alloc(DASICS_LIBCFG_V | DASICS_LIBCFG_R | DASICS_LIBCFG_W, (uint64_t)ulib2_rwbuffer, (uint64_t)(ulib2_rwbuffer + 128));
        idx3 = dasics_ulib_libcfg_copy(0);
        idx4 = dasics_ulib_libcfg_copy(1);
        extern char __ULIB2TEXT_BEGIN__, __ULIB2TEXT_END__;
        int32_t idx_ulib2 = dasics_ulib_jumpcfg_alloc(align8down((uint64_t)(&__ULIB2TEXT_BEGIN__)), align8up((uint64_t)&__ULIB2TEXT_END__));

        //CALL ULIB2
        dasics_ulib_libcall_no_args(&dasics_ulib2);

    #ifdef PRINT_DEBUG
        dasics_ulib1_printf((uint64_t)ulib1_rwbuffer);   
        dasics_ulib1_printf((uint64_t)ulib2_rwbuffer);    // That's ok
    #endif

        //FREE
        dasics_ulib_libcfg_free(idx4);
        dasics_ulib_libcfg_free(idx3);
        dasics_ulib_libcfg_free(idx1);
        dasics_ulib_libcfg_free(idx0);
        dasics_ulib_jumpcfg_free(idx_ulib2);
        uint64_t dasicsNestedAfterticks = readcycle;


        dasics_umaincall(Umaincall_PRINT, "> [RESULT] Nested use ticks: %d\n", dasicsNestedAfterticks - dasicsNestedPreticks);

    }
    
    // uint64_t dasicsNestedAfterticks = dasics_umaincall(Umaincall_GET_TICK);
    // uint64_t dasicsNestedAfterticks = dasics_umaincall(Umaincall_getclock);


}




void ATTR_ULIB1_TEXT dasics_ulib_maincall(void)
{

    // uint64_t dasicsMincallPreticks = dasics_umaincall(Umaincall_GET_TICK);
    // uint64_t dasicsMincallPreticks = dasics_umaincall(Umaincall_getclock);

    // uint64_t dasicsMincallPreticks = readcycle;


    for (int i = 0; i < 100; i++)
    {
        uint64_t dasicsMincallPreticks = readcycle;
    
        // Set new permission
    #ifdef PRINT_DEBUG
        dasics_ulib1_printf((uint64_t) ulib1_readonly);
        dasics_ulib1_printf((uint64_t) ulib1_rwbuffer);
    #endif

        permission_t permission;

        for (int i = 0; i < sizeof(permission_t); i++)
        {
            ((char *)(&permission))[i] = '\0';
        }
        
        // Copy the os field
        int libidx = 2;
        permission.dasicsLibBounds[2*libidx] = (uint64_t)ulib1_rwbuffer;
        permission.dasicsLibBounds[2*libidx + 1] = (uint64_t)ulib1_rwbuffer + 128;

        permission.dasicsLibCfg |= (DASICS_LIBCFG_V | DASICS_LIBCFG_R) << ( 4 * (libidx++));


        permission.dasicsLibBounds[2*libidx] = (uint64_t)ulib2_rwbuffer;
        permission.dasicsLibBounds[2*libidx + 1] = (uint64_t)ulib2_rwbuffer + 128;

        permission.dasicsLibCfg |= (DASICS_LIBCFG_V | DASICS_LIBCFG_R) << ( 4 * (libidx++));

        extern char __ULIB2TEXT_BEGIN__, __ULIB2TEXT_END__;

        int jmpidx = 0;
        permission.dasicsJumpBounds[2 * jmpidx] = align8down((uint64_t)(&__ULIB2TEXT_BEGIN__));
        permission.dasicsJumpBounds[2 * jmpidx + 1] = align8up((uint64_t)(&__ULIB2TEXT_END__));

        permission.dasicsJumpCfg |= (DASICS_JUMPCFG_V << (jmpidx++)*16);


        dasics_umaincall(Umaincall_switch, &permission, &dasics_ulib2);

    #ifdef PRINT_DEBUG
        dasics_ulib1_printf((uint64_t)ulib1_rwbuffer);   
        dasics_ulib1_printf((uint64_t)ulib2_rwbuffer);    // That's ok
    #endif

        uint64_t dasicsMincallAfterticks = readcycle;

        

        dasics_umaincall(Umaincall_PRINT, "> [RESULT] Mincall use ticks: %d\n", dasicsMincallAfterticks - dasicsMincallPreticks);

    }
    // uint64_t dasicsMincallAfterticks = dasics_umaincall(Umaincall_GET_TICK);
    // uint64_t dasicsMincallAfterticks = dasics_umaincall(Umaincall_getclock);
    // uint64_t dasicsMincallAfterticks = readcycle;

    

    // dasics_umaincall(Umaincall_PRINT, "> [RESULT] Mincall use ticks: 0x%lx\n", dasicsMincallAfterticks - dasicsMincallPreticks);

}

// #pragma GCC pop_options

const char * test_str = "RISCV";
const char * format_str = "[%s] Hello riscv world: %d!\n";
int main(int argc, char *argv[]) {
    // Add exit function 
    
    atexit(exit_function);
    register_udasics(0);


    int idx0, idx1, idx2, idx3, idx4;


    // Allocate libcfg before calling lib function
    idx0 = original_libcfg_alloc(DASICS_LIBCFG_V | DASICS_LIBCFG_R                  , (uint64_t)ulib1_readonly, (uint64_t)(ulib1_readonly + 128));
    idx1 = original_libcfg_alloc(DASICS_LIBCFG_V | DASICS_LIBCFG_R | DASICS_LIBCFG_W, (uint64_t)ulib1_rwbuffer, (uint64_t)(ulib1_rwbuffer + 128));
    // idx2 = original_libcfg_alloc(DASICS_LIBCFG_V                                    , (uint64_t)secret,       (uint64_t)(      secret + 128));
    idx4 = original_libcfg_alloc(DASICS_LIBCFG_V | DASICS_LIBCFG_R | DASICS_LIBCFG_W, (uint64_t)ulib2_rwbuffer, (uint64_t)(ulib2_rwbuffer + 128));

    extern char __ULIB1TEXT_BEGIN__, __ULIB1TEXT_END__;
    idx3 = original_jumpcfg_alloc(align8down((uint64_t)(&__ULIB1TEXT_BEGIN__)), align8up((uint64_t)&__ULIB1TEXT_END__));

    printf("> [INIT] Begin test exchange with dasics nested\n");
    
    dasics_umain_libcall_no_args(&dasics_ulib_nested);

    printf("> [INIT] Begin test exchange with dasics umaincall\n");

    dasics_umain_libcall_no_args(&dasics_ulib_maincall);
    

    original_libcfg_free(idx4);
    // Free those used libcfg via handlers
    // original_libcfg_free(idx2);
    original_libcfg_free(idx1);
    original_libcfg_free(idx0);

    unregister_udasics();
    return 0;
}