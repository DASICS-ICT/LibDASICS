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

#include <nginx_plugin.h>

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

void pcre_hook(struct cross *c, const char *target_name, struct umaincall * CallContext) {

    c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, pcre_heap_info.base, pcre_heap_info.size)

    if (!dasics_strcmp("pcre_compile", target_name)) {
        const char *pattern = (const char *)CallContext->a0;
        const char *errptr  = (const char *)CallContext->a2;
        int *erroffset      = (int *)CallContext->a3;
        const unsigned char *tableptr = (const unsigned char *)CallContext->a4;

        assert(c->handle_num + 4 <= MAX_BOUNS); // FIXME: no need to assert

        c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R, pattern, dasics_strlen(pattern) + 1);
        c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, errptr, sizeof(errptr));
        c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, erroffset, sizeof(int));

        if (tableptr != NULL) {
            c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, tableptr, dasics_strlen(tableptr) + 1);
        }
    } else if (!dasics_strcmp("pcre_exec", target_name)) {
        uint64_t *pcre      = (uint64_t *)CallContext->a0;
        uint64_t *pcre_ex   = (uint64_t *)CallContext->a1;
        const char *subject = (const char *)CallContext->a2;
        int *ovector        = (int *)CallContext->a6;

        assert(c->handle_num + 3 <= MAX_BOUNS);

        c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R, pcre, 0x70UL);
        c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R, subject, dasics_strlen(subject) + 1);
        c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, ovector, sizeof(int) * 64); // FIXME: the size is guessed

        if (pcre_ex != NULL) {
            c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R, pcre_ex, 64);
        }

    } else if (!dasics_strcmp("pcre_fullinfo", target_name)) {
        uint64_t *pcre      = (uint64_t *)CallContext->a0;
        uint64_t *pcre_ex   = (uint64_t *)CallContext->a1;
        void *where          = (int *)CallContext->a3;

        assert(c->handle_num + 2 <= MAX_BOUNS);

        c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R, pcre, 0x70UL);
        c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, where, sizeof(where));

        if (pcre_ex != NULL) {
            c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R, pcre_ex, 64);
        }
    } else if (!dasics_strcmp("pcre_study", target_name)) {
        uint64_t *pcre      = (uint64_t *)CallContext->a0;
        const char *errptr  = (const char *)CallContext->a2;

        c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R, pcre, 0x70UL);
        c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, errptr, sizeof(errptr));
    }
}

static void cross_call(umain_elf_t * _entry, umain_elf_t * _target, const char *name, struct umaincall * CallContext, uint64_t target_addr)
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

        struct cross *c = push_cross_get();
        // dasics_memset(&c, 0, sizeof(struct cross));
        c->begin = _entry;
        c->target = _target;
        c->ra = CallContext->ra;
        c->func = _target->namespace_func;

        // openssl hook
        if (target_addr >= openssl_area.text_begin && target_addr <= openssl_area.text_end) {

            c->jmpcfg[c->jmp_num++] = dasics_jumpcfg_alloc(openssl_area.text_begin, openssl_area.text_end);
            // for(int i = 0; i < openssl_area.rw_num; i++)
            // {
            //     c->handle[c->handle_num++] = dasics_libcfg_alloc(openssl_area.rw_bound[i].flags, \
            //                                 openssl_area.rw_bound[i].lo, \
            //                                 openssl_area.rw_bound[i].hi);
            // }
            c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, CallContext->sp - 16 * PAGE_SIZE, 16 * PAGE_SIZE);
            if (!openssl_area.is_active)
            {
                dasics_libcfg_active(openssl_area.longTimeHandle, openssl_area.longTimeHandle_num);
                openssl_area.is_active = 1;  
                c->clear_active = 1;
                dasics_memcpy(c->longTimeHandle, openssl_area.longTimeHandle, \
                                openssl_area.longTimeHandle_num * sizeof(int32_t));                              
            }

            c->longTimeHandle_num = openssl_area.longTimeHandle_num;
            goto hook_end;
        }
        
        c->jmpcfg[c->jmp_num++] = dasics_jumpcfg_alloc(_target->_plt_begin, _target->_text_end); // plt_begin -> text_end

        c->handle[c->handle_num++] = dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_V, \
                                        _target->_r_start,\
                                        _target->_r_end);
        c->handle[c->handle_num++] = dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_W | DASICS_LIBCFG_V, \
                                        _target->_w_start, \
                                        _target->_w_end);
        
        c->handle[c->handle_num++] = LIBCFG_ALLOC(DASICS_LIBCFG_R | DASICS_LIBCFG_W, CallContext->sp - 16 * PAGE_SIZE, 16 * PAGE_SIZE);

        // Hook pcre function
        pcre_hook(c, name, CallContext);
       
hook_end:
        // Push 
        // push_cross(&c);
        ;

    #ifdef DASICS_DEBUG
        dasics_printf("[LOG]: DASICS cross call (%s), return address: 0x%lx target elf: %s name: %s\n", _entry->real_name, CallContext->ra, _target->real_name, name);
    #endif

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
    // umain_elf_t * _elf = _get_area(CallContext->t1);
    // umain_elf_t * _elf = (umain_elf_t *)CallContext->t0;

    
    // Judge dynamic call
    // if (CallContext->t3 != (reg_t)dasics_umaincall)
    //     {
    //         // dasics umaincall return 
    //         if (CallContext->ra == (reg_t)dasics_umaincall) 
    //         {
    //             dasics_dynamic_return(CallContext);
    //             return 1;
    //         }
    //         return 0;
    //     }
        

    // dynamic_level++;

    assert(_elf->plt_begin != NULL);

    // int plt_idx = CallContext->t1 / 8;
    // Not Maincall
    // CallContext->t3 = 0;

    // Begin DASICS_ dynamic func 
    /* Result */ 
    // uint64_t target = 0;
    // umain_elf_t *target_elf = NULL;

    // // Now, we will got the target and so on 
    // target = _elf->_local_got_table[idx]; 
    // target_elf = _elf->target_elf[idx];    
    // const char * target_name = _elf->target_func_name[idx];

    // if (_elf->redirect_switch[idx] && redirect_switch)
    // {
    //     target = target - target_elf->l_addr + target_elf->_copy_lib_elf->l_addr;
    //     target_elf = target_elf->_copy_lib_elf;                
    // }

    CallContext->t1 = target;
    

//     if (handle_lib_mem(_elf, target_name, CallContext) == 0) 
//     { // successfully mem call
// #ifdef DASICS_DEBUG 
//         dasics_printf("[LOG]: %s:%s mem call\n", _elf->real_name, target_name);
// #endif    
//     } else 
//     {
//         cross_call(_elf, target_elf, target_name, CallContext);
//     }

    cross_call(_elf, target_elf, target_name, CallContext, target);

    // dynamic_level--;
    return 1;
}


void  dasics_dynamic_return(struct umaincall * CallContext)
{
    pop_cross(CallContext);
    
    // CallContext->t1 = CallContext->ra;

}


