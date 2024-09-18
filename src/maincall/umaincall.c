#include <umaincall.h>
#include <udasics.h>
#include <dynamic.h>
#include <udirect.h>
#include <dasics_start.h>
#include <dasics_stdio.h>
#include <dasics_string.h>
#include <cross.h>
#include <ufuncmem.h>
#include <mem.h>
#include <openssl_mem.h>
#include <ctype.h>
#include <errno.h>

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

    // umain_elf_t *_map = _umain_elf_table;

    // if (!_umain_elf_table) return 0;

    // do {   
    //     if ((_map ->_flags & LINK_AREA) || ((_map->_flags & MAIN_AREA) && !(_map->_flags & ELF_AREA)))
    //         goto giveup;

    //     /* got will be filled dasics_umaincall */
    //     for (int _i = 2; _i < _map->got_num; _i++)
    //     {            
    //         if (_map->got_begin[_i] != (uint64_t)_map->plt_begin) continue;
    //         _map->got_begin[_i] = (uint64_t)dasics_umaincall;
    //         _map->local_func[_i] = NULL;
    //     }              


    // giveup:
    //     _map = _map->umain_elf_next;
    // } while (_map != _umain_elf_table);

    return 0;
}


void cross_call(umain_elf_t * _entry, umain_elf_t * _target, const char *name, struct umaincall * CallContext)
{
    //TODO Add Cross-library calls here
    if ((_entry != _target) && \
        // &&// mean Cross call
        // !(
        //     (_entry->_flags & MAIN_AREA) &&
        //     (_target->_flags & MAIN_AREA)
        //  )                              // the entry and target both are trusted
        !(_target->_flags & MAIN_AREA) 
        // // &&
        // // !(_entry->_flags & MAIN_AREA)
        )
    {
        // dasics_printf("[LOG]: This is a cross call\n");
        static int openssl_flag = 0;
        if (openssl_flag == 0 && openssl_self_heap != NULL)
        {
            umain_elf_t * libcrypto = _get_area_by_name("libcrypto.so.1.0.0");
            umain_elf_t * libssl = _get_area_by_name("libssl.so.1.0.0");
            umain_elf_t * libc = _get_area_by_name("libc.so.6");        
            assert(libcrypto != NULL);
            assert(libssl != NULL);
            assert(libc != NULL); 
            register void *tp asm ("tp");

            dasics_printf("Set openssl self heap begin: 0x%lx, end: 0x%lx\n", (uint64_t)openssl_self_heap, (uint64_t)openssl_self_heap + openssl_full_size);
            dasics_printf("Set TLS begin: 0x%lx, end: 0x%lx\n", tp, tp + PAGE_SIZE);
            LIBCFG_ALLOC(DASICS_LIBCFG_W | DASICS_LIBCFG_R | DASICS_LIBCFG_V, (uint64_t)openssl_self_heap, openssl_full_size);
            LIBCFG_ALLOC(DASICS_LIBCFG_W | DASICS_LIBCFG_R | DASICS_LIBCFG_V, tp, PAGE_SIZE);
            LIBCFG_ALLOC(DASICS_LIBCFG_W | DASICS_LIBCFG_R | DASICS_LIBCFG_V, libc->_r_start, libc->_w_end);
            dasics_printf("Set openssl library data: 0x%lx, 0x%lx\n", libcrypto->_r_start, libssl->_w_end);
            LIBCFG_ALLOC(DASICS_LIBCFG_W | DASICS_LIBCFG_R | DASICS_LIBCFG_V, libcrypto->_r_start, libssl->_w_end);
            dasics_jumpcfg_alloc(libcrypto->_r_start, libssl->_w_end);
            openssl_flag = 1;
        }

        struct cross tmp;
        int idx_lib = 0;
        int idx_jmp = 0;
        dasics_memset(&tmp, 0, sizeof(struct cross));
        tmp.begin = _entry;
        tmp.target = _target;
        tmp.ra = CallContext->ra;
        tmp.func = _target->namespace_func;
        
        // tmp.jmpcfg[idx_jmp++] = dasics_jumpcfg_alloc(_target->_plt_start, _target->_text_end);

        // tmp.handle[idx_lib++] = dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_V, \
        //                                 _target->_r_start,\
        //                                 _target->_r_end);
        // tmp.handle[idx_lib++] = dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_W | DASICS_LIBCFG_V, \
        //                                 _target->_w_start, \
        //                                 _target->_w_end);
        
        // tmp.handle[idx_lib++] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, CallContext->sp - 16 * PAGE_SIZE, 16 * PAGE_SIZE);

        tmp.handle_num = idx_lib;
        
        // Push 
        push_cross(&tmp);
        
        // dasics_printf("[LOG]: DASICS lib (%s), return address: 0x%lx target elf: %s name: %s\n", _entry->real_name, CallContext->ra, _target->real_name, name);

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
    // umain_elf_t * _elf = _get_area(CallContext->t1);
    umain_elf_t * _elf = (umain_elf_t *)CallContext->t0;

    
    // Judge dynamic call
    if (CallContext->a0 == DASICS_HOOK_MAGIC || \
        !_elf || \
        CallContext->t3 != (reg_t)dasics_umaincall)
        {
            // dasics umaincall return 
            if (CallContext->ra == (reg_t)dasics_umaincall) 
            {
                dasics_dynamic_return(CallContext);
                return 1;
            }
            return 0;
        }
        

    dynamic_level++;

    assert(_elf->plt_begin != NULL);

    // int plt_idx = _is_plt_area(CallContext->t1, _elf);
    int plt_idx = CallContext->t1 / 8;
    // Not Maincall
    CallContext->t3 = 0;

    if (plt_idx == -1) 
    {
        dasics_printf("[ERROR]: DASICS ERROR, dasics_dynamic error\n");
        exit(1);
    } 
    // Begin DASICS_ dynamic func 
    /* Result */ 
    uint64_t target = 0;
    umain_elf_t *target_elf = NULL;
    const char * target_name = NULL;

    // Now, we will got the target and so on 
    target = _elf->_local_got_table[plt_idx + 2]; 
    target_elf = _elf->target_elf[plt_idx + 2];    
    target_name = _elf->target_func_name[plt_idx + 2]; 
    assert(target != 0);
    assert(target_elf != NULL); 
    assert(target_name != NULL);


    CallContext->t1 = target;
    if (_elf == _umain_elf_table && !(target_elf->_flags & MAIN_AREA))
    {
        csr_write(0x8b4, CallContext->ra);
        asm("fence.i");
    }
        



    // Add memory support
    // if (!(target_elf->_flags & MAIN_AREA) && target_elf != _elf)
    // {
    //     if (_elf->local_func[plt_idx + 2])
    //         target_elf->namespace_func = _elf->local_func[plt_idx + 2];
    //     else 
    //     {
    //         set_global_func_man(target_elf, target);
    //         // Next time 
    //         _elf->local_func[plt_idx + 2] = target_elf->namespace_func; 
    //     }
    // }

    // dasics_printf("[LOG]: DASICS lib (%s), target elf: %s name: %s\n", _elf->real_name, target_elf->real_name, target_name);

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


