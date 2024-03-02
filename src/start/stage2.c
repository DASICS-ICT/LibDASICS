#include <dasics_start.h>
#include <udasics.h>
#include <dynamic.h>
#include <dasics_stdio.h>
#include <umaincall.h>

fixup_entry_t dll_fixup_handler;

rtld_fini dll_fini;

/*
 * for the stage two, we will set a utrap function 
 * for the stage3, and some prepare
 */
void _dasics_entry_stage2(uint64_t sp, rtld_fini fini)
{
    // csr_write(0x005, (uint64_t)dasics_ufault_entry);

    // Get FIXUP
    dll_fixup_handler = (fixup_entry_t)_get_auxv_entry(sp, AT_FIXUP);
    dll_fini = fini;

    user_sp = sp;

    dasics_printf("> [INIT] _dasics_entry_stage2\n");
    
    // get the struct link_map
    Elf64_Dyn * dyn =  NULL;
    struct r_debug * debug_extended =  NULL;
    for(dyn = _DYNAMIC; dyn->d_tag != DT_NULL; ++dyn)
    {
        if(dyn->d_tag == DT_DEBUG)
        {
            debug_extended = (struct r_debug *)dyn->d_un.d_ptr;
        }          
    }
    
    struct link_map * link = debug_extended->r_map;

    for (struct link_map *cur = link; cur != NULL ; cur = cur->l_next)
    {
        dasics_printf("[LIB]: %s, l_addr: 0x%lx\n", cur->l_name, cur->l_addr);
    }

    create_umain_elf_chain(link);
// #ifdef DASICS_LINUX
//     // for (int i = 0; i < 2 * DASICS_LIBCFG_WIDTH; i++)
//     // {
//     //     // clear all dasics lib bounds which used on the dynamic link time
//     //     dasics_libcfg_free(i);
//     // }
//     csr_write(0x880, 0);
//     csr_write(0x8c8, 0);
// #endif




//     /* create got chain to support dynamic call  */
//     create_umain_got_chain(main_map, *(char**)(sp + 8));


#ifdef DASICS_LINUX
//     /* open maincall for dynamic if you need, or we will used ufault exception */
//     _open_maincall();
//     /* open dynamic's got read jurisdiction */
//     dasics_libcfg_alloc(DASICS_LIBCFG_V | DASICS_LIBCFG_R, \
//                         (uint64_t)_umain_got_table->got_begin, \
//                         (uint64_t)_umain_got_table->got_begin + sizeof(uint64_t) * (_umain_got_table->got_num + 2));
//     dasics_libcfg_alloc(DASICS_LIBCFG_V | DASICS_LIBCFG_R | DASICS_LIBCFG_W, \
//                 TASK_SIZE/2, \
//                 TASK_SIZE);
//     dasics_jumpcfg_alloc(TASK_SIZE/2, TASK_SIZE);
//     my_printf("> [INIT] Init maincall for dynamic successfully\n");
#endif
    _open_maincall();

// #ifdef DASICS_DEBUG

//     umain_got_t * _local_umain_map = _umain_got_table;
//     while (_local_umain_map)
//     {
//         debug_print_umain_map(_local_umain_map);
//         my_printf("\n\n");
//         _local_umain_map = _local_umain_map->umain_got_next;
//     }

// #endif

    dasics_stage = 2;
    

// #ifdef DASICS_COPY
//     /* begin to init copy of the trust lib */
//     uint64_t copy_linker_dll = _get_auxv_entry(sp, AT_LINKER_COPY);
//     #ifdef DASICS_DEBUG
//         my_printf("> [INIT] AT_LINKER_COPY: 0x%lx\n", copy_linker_dll);
//     #endif
//     // Open data segment to read
//     open_memory(_umain_got_table);
//     // Register eixt func to 
//     if (fini)
//         atexit(fini);
//     /* change dasics_flag to 2 let linker just map trust lib on untrusted area*/
//     _set_auxv_entry(sp, AT_DASICS, 2);

//     /* Go to stage 3 */
//     _set_auxv_entry(sp, AT_ENTRY, (uint64_t)_copy_lib_entry);

//     csr_write(0x005, (uint64_t)__dasics_start_ufault_entry);
//     RESET_ENTRY(sp, copy_linker_dll);

// #endif

// #ifdef DASICS_LINUX

//     check_copy_library();

// #endif
}