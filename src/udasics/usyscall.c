#include <usyscall.h>
#include <dasics_stdio.h>
#include <stdlib.h>

ecall_check_t syscall_check[__NR_syscalls];

/*  
 * default no check for the ecall 
 */
int default_ecall_check_handler(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7)
{
    // #ifdef DASICS_DEBUG
        dasics_printf("[DEBUG]: catch user syscall %d\n", a7);
    // #endif
    return 0;    
}


/* default ecall error handler */
int default_ecall_error_handler()
{
    return 0;
}

/* init all syscall's check handler */
void init_syscall_check()
{
    for (int i = 0; i < __NR_syscalls; i++)
    {
        syscall_check[i].check = (int (*)())default_ecall_check_handler;
        syscall_check[i].handle_error = (int (*)())default_ecall_error_handler;
    }
    
}