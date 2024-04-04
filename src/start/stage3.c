#include <dasics_start.h>
#include <dasics_stdio.h>
#include <dynamic.h>
#include <udasics.h>
#include <umaincall.h>
#include <cross.h>

// STD
#include <stdlib.h>

fixup_entry_t dll_fixup_handler_lib = NULL;


void _dasics_entry_stage3(uint64_t sp, rtld_fini fini)
{
#ifdef DASICS_DEBUG
    dasics_printf("> [INIT] _dasics_entry_stage3\n");
#endif    

    dll_fixup_handler_lib = (fixup_entry_t)_get_auxv_entry(sp, AT_FIXUP);

    struct link_map * link = get_main_link();
    create_umain_elf_chain(link);


    /* set copy lib's GOT to dasics_umain_call */
    _open_maincall();

#ifdef DASICS_DEBUG
    dasics_printf("> [INIT] Init maincall for dynamic successfully\n");
#endif

    /* Add copy ld.so to atexit */
    // if (fini)
    //     atexit(fini);

#ifdef DASICS_DEBUG
    dasics_printf("> [INIT] Add func 0x%lx to exit chain\n", fini);    
#endif

#ifdef DASICS_LINUX
    // Clear all lib bounds
    csr_write(0x880, 0);
    csr_write(0x8c8, 0);

    original_libcfg_free_all();
    original_jumpcfg_free_all();

    /* open dynamic's got read jurisdiction */
    original_libcfg_alloc(DASICS_LIBCFG_V | DASICS_LIBCFG_R, \
                        (uint64_t)_umain_elf_table->got_begin, \
                        (uint64_t)_umain_elf_table->got_begin + sizeof(uint64_t) * (_umain_elf_table->got_num + 2));
    original_libcfg_alloc(DASICS_LIBCFG_V | DASICS_LIBCFG_R | DASICS_LIBCFG_W, \
                0, \
                TASK_SIZE);
    original_jumpcfg_alloc(TASK_SIZE/2, TASK_SIZE);

    // setup user ufault handler 
    csr_write(0x005, (uint64_t)dasics_ufault_entry);

#endif
}