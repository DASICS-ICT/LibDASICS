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

int dynamic_level = 0;
uint64_t memset_num = 0;
uint64_t memcpy_num = 0;

int _open_maincall()
{
    umaincall_helper = (uint64_t)dasics_umaincall_helper;
    csr_write(0x8b0, (uint64_t)dasics_umaincall);
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
        /* 0 is a valid DASICS handle index; use -1 as invalid sentinel. */
        for (int i = 0; i < DASICS_JUMPCFG_WIDTH; i++)
            tmp.jmpcfg[i] = -1;
        for (int i = 0; i < DASICS_LIBCFG_WIDTH; i++)
            tmp.handle[i] = -1;
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
        
        tmp.handle[idx_lib++] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, CallContext->sp - 16 * PAGE_SIZE, 16 * PAGE_SIZE);

        tmp.handle_num = idx_lib;
        
        // Push 
        push_cross(&tmp);
        
        // dasics_printf("[LOG]: DASICS lib (%s), return address: 0x%lx target elf: %s name: %s\n", _entry->real_name, CallContext->ra, _target->real_name, name);

        CallContext->ra = (reg_t)dasics_umaincall;
  
    }
}


    static int openssl_flag = 0;


int dasics_dynamic_call(struct umaincall * CallContext, \
                            umain_elf_t * _elf, \
                            int idx, \
                            uint64_t target, \
                            umain_elf_t * target_elf, \
                            const char * target_name)
{
    assert(_elf->plt_begin != NULL);
    // redirect "Maybe it will be removed"
    if (_elf->redirect_switch[idx + 2] && redirect_switch)
    {
        target = target - target_elf->l_addr + target_elf->_copy_lib_elf->l_addr;
        target_elf = target_elf->_copy_lib_elf;                
    }

    CallContext->t1 = target;
    
    cross_call(_elf, target_elf, target_name, CallContext);

    return 1;
}


void  dasics_dynamic_return(struct umaincall * CallContext)
{
    pop_cross(CallContext);
    
    CallContext->t1 = CallContext->ra;

}


