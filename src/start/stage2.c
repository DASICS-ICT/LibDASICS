#include <dasics_start.h>
#include <dasics_stdio.h>
#include <udasics.h>
#include <dynamic.h>
#include <umaincall.h>
#include <cross.h>

void print_exit_func_num();
// STD
#include <stdlib.h>

fixup_entry_t dll_fixup_handler;

rtld_fini dll_fini;

/*
 * for the stage two, we will set a utrap function 
 * for the stage3, and some prepare
 */
void _dasics_entry_stage2(uint64_t sp, rtld_fini fini)
{
#ifdef DASICS_DEBUG
    dasics_printf("> [INIT] _dasics_entry_stage2\n");    
#endif
    user_sp = sp;

    struct link_map * link = get_main_link();

    // Get FIXUP
#ifdef DASICS_LINUX
    dll_fixup_handler = (fixup_entry_t)_get_auxv_entry(sp, AT_FIXUP);
    dll_fini = fini;
#else
    struct link_map * tmp = link;
    extern char _interp_start[];
    for (; tmp != 0; tmp = tmp->l_next)
    {
        /* code */
        if (!dasics_strcmp(tmp->l_name, _interp_start))
        {
            dll_fixup_handler = (fixup_entry_t)(tmp->l_addr + 0xac66);
        }
    }
#endif

#ifdef DASICS_DEBUG
    dasics_printf("> [INIT] dll_fixup_handler: 0x%lx\n", (uint64_t)dll_fixup_handler);
#endif

    create_umain_elf_chain(link);
    
    
    /* open maincall for dynamic */
    _open_maincall();

#ifdef DASICS_DEBUG
    dasics_printf("> [INIT] Init maincall for dynamic successfully\n");
#endif
    init_cross_stack();

// #ifdef DASICS_DEBUG
//     dasics_printf("> [INIT] Init corss stack successfully\n");
// #endif

#ifdef DASICS_COPY
    /* begin to init copy of the trust lib */
    uint64_t copy_linker_dll = _get_auxv_entry(sp, AT_LINKER_COPY);
    
    if (!copy_linker_dll) return;

    // Open data segment to read
    open_memory(_umain_elf_table);
    // Register eixt func to 
    if (fini)
        atexit(fini);
    /* change dasics_flag to 2 let linker just map trust lib on untrusted area*/
    _set_auxv_entry(sp, AT_DASICS, 2);
    // Stage 2
    dasics_stage = 2;
    /* Go to stage 3 */
    _set_auxv_entry(sp, AT_ENTRY, (uint64_t)_setup_copy_lib_entry);

    csr_write(0x005, (uint64_t)_setup_fault);
    RESET_ENTRY(sp, copy_linker_dll);

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
    // setup user ufault handler 
    csr_write(0x005, (uint64_t)dasics_ufault_entry);

    atexit(&print_exit_func_num);
    _umain_elf_table->calculate = 1;

#endif

    
}

void print_exit_func_num()
{
    // Close calculate
    _umain_elf_table->calculate = 0;

    umain_elf_t *elf = _umain_elf_table;
    
    printf("[LOG]: numbers of func call\n");
    for (int i = 2; i < elf->got_num + 2; i++)
    {
        /* code */
        printf(" > func:%-*s addr: 0x%-*lx numbers: %-*ld\n",\
                20, _get_lib_name(elf, i -2), \
                18,  elf->_local_got_table[i], \
                32, elf->_local_call_time[i]);
    }

}