#include <ucsr.h>
#include <dasics_start.h>
#include <dasics_stdio.h>
#include <usyscall.h>
#include <stdlib.h>
#include <udasics.h>

extern uint64_t umaincall_helper;
uint64_t user_sp = 0;
/* 
 * if we get the _dll_linker from the auxv which means that 
 * the program need a dynamic linker for stage one, we will 
 * set a utrap function for the dynamic linker to execute 
 * the init function of the library.
 * 
 * or we will go to stage two directly
 */
void _dasics_entry_stage1(uint64_t sp, rtld_fini fini)
{
#ifdef DASICS_DEBUG
    dasics_printf("> [INIT] _dasics_entry_stage1\n");
#endif    
    uint64_t _dll_linker = _get_auxv_entry(sp, AT_LINKER);

    user_sp = sp;

    if (_dll_linker && _get_auxv_entry(sp, AT_DASICS))
    {
        // change the elf_enrtry to  _umain_entry
        _set_auxv_entry(sp, AT_ENTRY, (uint64_t)_setup_mainlib_entry);

        dasics_stage = 1;

        init_syscall_check();
#ifdef DASICS_DEBUG
        dasics_printf("> [INIT] Init syscall_check_table successfully\n");        
#endif

        _set_auxv_entry(sp, AT_DASICS, 1);
        /* 
         * give all lib area be VALID, READ, WRITE, FREE
         * This is not safe
         */
        original_libcfg_alloc(DASICS_LIBCFG_V | DASICS_LIBCFG_R | DASICS_LIBCFG_W, \
                    0, \
                    TASK_SIZE);
        original_jumpcfg_alloc(0, TASK_SIZE);

        // set a trap entry for link time
        csr_write(0x005, (uint64_t)_setup_fault);
        // Transfer executive authority to dynamic linker
        RESET_ENTRY(sp, _dll_linker);
    }
    umaincall_helper = (uint64_t)dasics_umaincall_helper;
    csr_write(0x8b0, (uint64_t)dasics_umaincall);

    if (_dll_linker)
    {
        if (fini) atexit(fini);
    }
}

