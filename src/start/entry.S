#include <asm/asm.h>
#inlcude <asm/offset.h>

// True entry of dasics excution
ENTRY(_check_dasics)
    /* Load the global pointer*/  
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop  
    
    /* Maybe no dasics but have a dll_exit */
    mv a1, a0
    mv a0, sp
    call _dasics_entry_stage1
    li a0, 0

    call _start
END(_check_dasics)


ENTRY(_setup_mainlib_entry)
    /* Load the global pointer*/  
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop  

    /* 
     * we must save a0 for the func 
     * before we enter the _start
     */
    mv a1, a0
    mv a0, sp
    call _dasics_entry_stage2
    li a0, 0
    call _start
END(_setup_mainlib_entry)

ENTRY(_setup_copy_lib_entry)
    /* Load the global pointer*/  
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop  

    /* 
     * we must save a0 for the func 
     * before we enter the _start
     */
    mv a1, a0
    mv a0, sp
    // call _dasics_entry_stage3
    li a0, 0
    call _start
END(_setup_copy_lib_entry)



