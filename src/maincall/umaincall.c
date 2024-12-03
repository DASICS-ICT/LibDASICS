#include <umaincall.h>
#include <udasics.h>
#include <dynamic.h>
#include <udirect.h>
#include <dasics_start.h>
#include <dasics_stdio.h>
#include <dasics_string.h>
#include <cross.h>
#include <ufuncmem.h>
#include <ctype.h>
#include <errno.h>

// STD
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define GOT_NUM 4096
long long int dynamic_level = -1;
uint64_t memset_num = 0;
uint64_t memcpy_num = 0;
uint64_t _local_table[GOT_NUM] = {0};

int _open_maincall()
{
    umaincall_helper = (uint64_t)dasics_umaincall_helper;
    csr_write(0x8b0, (uint64_t)dasics_umaincall);
    // _local_table = _umain_elf_table->_local_got_table;
    int copy_size = sizeof(uint64_t) * (_umain_elf_table ->got_num + 2);
    assert((_umain_elf_table ->got_num + 2) < GOT_NUM);
    dasics_memcpy(_local_table, _umain_elf_table->_local_got_table, copy_size);
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
        csr_write(0x8b1 , CallContext->ra);

    }
}


    static int openssl_flag = 0;


int dasics_dynamic_call(struct umaincall * CallContext)
{
    // umain_elf_t * _elf = _get_area(CallContext->t1);
    umain_elf_t * _elf = (umain_elf_t *)CallContext->t0;

    
    // Judge dynamic call
    if (CallContext->t3 != (reg_t)dasics_umaincall)
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

    int plt_idx = CallContext->t1 / 8;
    // Not Maincall
    CallContext->t3 = 0;

    // Begin DASICS_ dynamic func 
    /* Result */ 
    uint64_t target = 0;
    umain_elf_t *target_elf = NULL;

    // Now, we will got the target and so on 
    target = _elf->_local_got_table[plt_idx + 2]; 
    target_elf = _elf->target_elf[plt_idx + 2];    
    const char * target_name = _elf->target_func_name[plt_idx + 2];

    // dasics_printf("%s: %s\n", __func__, target_name);

    if (_elf->redirect_switch[plt_idx + 2] && redirect_switch)
    {
        target = target - target_elf->l_addr + target_elf->_copy_lib_elf->l_addr;
        target_elf = target_elf->_copy_lib_elf;                
    }

    CallContext->t1 = target;
    


    cross_call(_elf, target_elf, target_name, CallContext);


    dynamic_level--;
    return 1;
}


void  dasics_dynamic_return(struct umaincall * CallContext)
{
    pop_cross(CallContext);
    
    CallContext->t1 = CallContext->ra;

}


