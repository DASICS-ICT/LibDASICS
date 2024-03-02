#include <umaincall.h>
#include <udasics.h>
#include <dynamic.h>
#include <dasics_start.h>
#include <dasics_stdio.h>

static uint64_t emulate_plt(uint32_t * pltPc, uint64_t * gotAddr)
{
/*
00000000010069a0 <malloc@plt>:
 10069a0:	00002e17          	auipc	t3,0x2
 10069a4:	6d0e3e03          	ld	t3,1744(t3) # 1009070 <malloc@GLIBC_2.27>
 10069a8:	000e0367          	jalr	t1,t3
 10069ac:	00000013
*/  
    uint32_t auipc = *pltPc;
    uint32_t ld = *(pltPc + 1);
    uint32_t jalr = *(pltPc + 2);

#define IMM_LD   0xfff00000UL
#define IMM_AUIPC      0xfffff000UL

    // uint64_t gotTarget = (uint64_t)pltPc + (int64_t)()
    int64_t auipc_imm = (int64_t)(auipc & IMM_AUIPC);

    uint64_t t3 = (uint64_t)pltPc + auipc_imm;
    int32_t ld_imm = (int32_t)((int16_t)((ld & IMM_LD)>> 20));

    // This load addr of t3
    uint64_t addr = t3 + ld_imm;

    return (uint64_t)pltPc - 0x10 * ((addr - (uint64_t)gotAddr) / 8);

}

int _open_maincall()
{
    umaincall_helper = (uint64_t)dasics_umaincall_helper;
    
    umain_elf_t *_map = _umain_elf_table;


    do {   
        if (_map ->_flags & LINK_AREA)
            goto giveup;

        /* got will be filled dasics_umaincall */
        for (int _i = 2; _i < _map->got_num; _i++)
        {            
            _map->got_begin[_i] = (uint64_t)dasics_umaincall;
        }              


    giveup:
        _map = _map->umain_elf_next;
    } while (_map != _umain_elf_table);

}


int dasics_dynamic_call(struct umaincall * CallContext)
{
    
    umain_elf_t * _elf = _get_area(CallContext->t1);
    
    // Judge dynamic call
    if (!_elf || *(uint32_t *)CallContext->t1 != NOP) return 1;

    // Calculate plt begin
    if (!_elf->plt_begin) {
        _elf->_plt_start = emulate_plt((uint32_t *)(CallContext->t1 - 0xc), \
                                (uint64_t *)_elf->got_begin);
        _elf->_plt_end = _elf->_plt_start + 0x10 * _elf->got_num;
        _elf->plt_begin = (uint64_t *)_elf->_plt_start;
    }

    dasics_printf("[LOG]: lib (%s)'s plt begin: 0x%lx\n", _elf->real_name, _elf->_plt_start);

    dasics_printf("[LOG]: Handle dynamic call\n");

    int plt_idx = _is_plt_area(CallContext->t1, _elf);

    if (plt_idx == -1) 
    {
        dasics_printf("[ERROR]: DASICS ERROR, dasics_dynamic error\n");
        while(1);
    } 

    // Begin DASICS_ dynamic func 
    /* Result */ 
    uint64_t target = 0;
    umain_elf_t *target_elf = NULL;

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
        uint64_t dll_a0 = _elf->map_link;
        uint64_t dll_a1 = (((reg_t)plt_idx * 0x10UL) >> 1) * 3;
        uint64_t ulib_func = _elf->fixup_handler(dll_a0, dll_a1);
        ulib_func = _call_reloc(_elf, ulib_func);
        // change the target by user force
        uint64_t force_func = force_redirect(_elf, _get_lib_name(_elf, plt_idx), ulib_func);

        // update the true target
        target_elf = _get_trap_area(force_func);
        target = CallContext->t1 = force_func;

        /* saved */
        _elf->_local_got_table[plt_idx + 2] = ulib_func;

        /* reset got */
        _elf->got_begin[plt_idx + 2] = (uint64_t)dasics_umaincall;
    } else 
    {
        /**
         * Now, the got has been filled with the lib function address in the memory
         * we will check it.
         */
        target = force_redirect(_elf, _get_lib_name(_elf, plt_idx),  _elf->_local_got_table[plt_idx + 2]);
        CallContext->t1 = target;
        target_elf = _get_trap_area(target);

    }

    while(1);
}