#include <umaincall.h>
#include <udasics.h>
#include <dynamic.h>
#include <udirect.h>
#include <dasics_start.h>
#include <dasics_stdio.h>
#include <dasics_string.h>
#include <cross.h>
#include <ufuncmem.h>

// STD
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int dynamic_level = 0;
uint64_t memset_num = 0;
uint64_t memcpy_num = 0;

int _open_maincall()
{
    umaincall_helper = (uint64_t)dasics_umaincall_helper;
    csr_write(0x8b0, (uint64_t)dasics_umaincall);

    umain_elf_t *_map = _umain_elf_table;

    if (!_umain_elf_table) return 0;

    do {   
        if ((_map ->_flags & LINK_AREA) || ((_map->_flags & MAIN_AREA) && !(_map->_flags & ELF_AREA)))
            goto giveup;

        /* got will be filled dasics_umaincall */
        for (int _i = 2; _i < _map->got_num; _i++)
        {            
            _map->got_begin[_i] = (uint64_t)dasics_umaincall;
            _map->local_func[_i] = NULL;
        }              


    giveup:
        _map = _map->umain_elf_next;
    } while (_map != _umain_elf_table);

    return 0;
}


void cross_call(umain_elf_t * _entry, umain_elf_t * _target, const char *name, struct umaincall * CallContext)
{

    //TODO Add Cross-library calls here
    if ((_entry != _target) &&  // mean Cross call
        !(
            (_entry->_flags & MAIN_AREA) &&
            (_target->_flags & MAIN_AREA)
         )                              // the entry and target both are trusted
        )
    {
        // dasics_printf("[LOG]: This is a cross call\n");
        struct cross tmp;
        int idx_lib = 0;
        int idx_jmp = 0;
        dasics_memset(&tmp, 0, sizeof(struct cross));
        tmp.begin = _entry;
        tmp.target = _target;
        tmp.ra = CallContext->ra;
        tmp.func = _target->namespace_func;
        
        tmp.jmpcfg[idx_jmp++] = dasics_jumpcfg_alloc(_target->_plt_start, _target->_text_end);

        tmp.handle[idx_lib++] = dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_V, \
                                        _target->_r_start,\
                                        _target->_r_end);
        tmp.handle[idx_lib++] = dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_W | DASICS_LIBCFG_V, \
                                        _target->_w_start, \
                                        _target->_w_end);
        
        tmp.handle[idx_lib++] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, CallContext->sp - 4 * PAGE_SIZE, 4 * PAGE_SIZE);
        // if (!dasics_strcmp(name, "memset"))
        // {
        //     memset_num++;
        //     tmp.handle[idx_lib++] = LIBCFG_ALLOC(DASICS_LIBCFG_W | DASICS_LIBCFG_R, CallContext->a0, CallContext->a2)
        // }

        // if (!dasics_strcmp(name, "memcpy"))
        // {
        //     memcpy_num++;
        //     // dasics_printf("dst: 0x%lx, src: 0x%lx, length: 0x%lx\n", CallContext->a0, CallContext->a1, CallContext->a2);
        //     tmp.handle[idx_lib++] = LIBCFG_ALLOC(DASICS_LIBCFG_W | DASICS_LIBCFG_R, CallContext->a0, CallContext->a2);
        //     tmp.handle[idx_lib++] = LIBCFG_ALLOC(DASICS_LIBCFG_R, CallContext->a1, CallContext->a2);
        // }

        tmp.handle_num = idx_lib;

        // Push 
        push_cross(&tmp);

        CallContext->ra = (reg_t)dasics_umaincall;

        if (!_target->namespace_func) return;
        // Alloc bound
        struct bound_table * bounds = _target->namespace_func->mem;

        for (int i = 0; i <_target->namespace_func->bound_max; i++)
        {
            /* code */
            if (bounds[i].addr)
                bounds[i].handler = LIBCFG_ALLOC(DASICS_LIBCFG_W | DASICS_LIBCFG_V, bounds[i].addr, bounds[i].length);
        }        
    }
}




int dasics_dynamic_call(struct umaincall * CallContext)
{
    if (!_umain_elf_table) return 0;

    // dasics umaincall return 
    if (CallContext->ra == (reg_t)dasics_umaincall) 
    {
        dasics_dynamic_return(CallContext);
        return 1;
    }

    umain_elf_t * _elf = _get_area(CallContext->t1);
    
    // Judge dynamic call
    if (!_elf || *(uint32_t *)CallContext->t1 != NOP) return 0;

    dynamic_level++;


    int plt_idx = _is_plt_area(CallContext->t1, _elf);

    if (plt_idx == -1) 
    {
        dasics_printf("[ERROR]: DASICS ERROR, dasics_dynamic error\n");
        exit(1);
    } 
    // Begin DASICS_ dynamic func 
    /* Result */ 
    uint64_t target = 0;
    umain_elf_t *target_elf = NULL;
    const char * target_name = _get_lib_name(_elf, plt_idx);

    if (!handle_lib_mem(_elf, target_name, CallContext))
    {
        return 1;
    }

    /* First time call */ 
    if (!_elf->_local_got_table[plt_idx + 2])
    {
        /*
        * We found that the Plt[x] wants to use dely binding to find the fucntion,
        * and we prepare all the parameters, and jump
        * 
        * dll_a0: the got[1], struct link_map of the library
        * dll_a1: the thrice of the plt table offset
        * ulib_func: the addr of the ulib function 
        */
        uint64_t dll_a0 = (uint64_t)_elf->map_link;
        uint64_t dll_a1 = (((reg_t)plt_idx * 0x10UL) >> 1) * 3;
        uint64_t ulib_func = _elf->fixup_handler(dll_a0, dll_a1);
        ulib_func = _call_reloc(_elf, ulib_func);
        // change the target by user force
        uint64_t force_func = ulib_func;
        // if (dynamic_level == 1)
        force_func = force_redirect(_elf, plt_idx, ulib_func);

        target_elf = _get_area(force_func);
        target = force_func;
        
        /* saved */
        _elf->_local_got_table[plt_idx + 2] = target;
        _elf->target_elf[plt_idx + 2] = target_elf;

        // Dasics call no need to redirect
        if (dynamic_level != 1 && _elf->redirect_switch[plt_idx + 2])
        {
            if (target_elf->_copy_lib_elf)
            {
                target = target - target_elf->l_addr + target_elf->_copy_lib_elf->l_addr;
                target_elf = target_elf->_copy_lib_elf;                
            }
        }

        /* reset got */
        _elf->got_begin[plt_idx + 2] = (uint64_t)dasics_umaincall;
    } else 
    {
        /**
         * Now, the got has been filled with the lib function address in the memory
         * we will check it.
         */
        target = _elf->_local_got_table[plt_idx + 2]; 
        target_elf = _elf->target_elf[plt_idx + 2];     

        // Dasics call no need to redirect
        if (dynamic_level != 1 && _elf->redirect_switch[plt_idx + 2])
        {
            if (target_elf->_copy_lib_elf)
            {
                target = target - target_elf->l_addr + target_elf->_copy_lib_elf->l_addr;
                target_elf = target_elf->_copy_lib_elf;                
            }
        } 
    }

    CallContext->t1 = target;



    // Add memory support
    if (!(target_elf->_flags & MAIN_AREA) && target_elf != _elf)
    {
        if (_elf->local_func[plt_idx + 2])
            target_elf->namespace_func = _elf->local_func[plt_idx + 2];
        else 
        {
            set_global_func_man(target_elf, target);
            // Next time 
            _elf->local_func[plt_idx + 2] = target_elf->namespace_func; 
        }
    }

    // dasics_printf("[LOG]: DASICS lib (%s), name: %s\n", _elf->real_name, _get_lib_name(_elf, plt_idx));

    cross_call(_elf, target_elf, target_name, CallContext);


    dynamic_level--;
    return 1;
}


void  dasics_dynamic_return(struct umaincall * CallContext)
{
    // dasics_printf("[LOG]: This is a dynamic return\n");

    pop_cross(CallContext);
    
    CallContext->t1 = CallContext->ra;

}


