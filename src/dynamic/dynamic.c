#include <elf.h>
#include <link.h>
#include <stddef.h>

#include <list.h>
#include <dynamic.h>
#include <dasics_string.h>
#include <dasics_stdio.h>
#include <dasics_start.h>
#include <dmalloc.h>
#include <udasics.h>

//STD
#include <stdlib.h>

umain_elf_t * _umain_elf_table = NULL;
umain_elf_t * dasics_main_elf = NULL;
int dasics_stage = 0;

extern char _interp_start[];

// Fill dynamic section
static void _fill_dynamic_section(ElfW(Dyn) **l_info, ElfW(Dyn) *l_ld)
{
    Elf64_Dyn * dyn =  NULL;

    for (dyn = l_ld; dyn->d_tag != DT_NULL; dyn++)
    {   
        if (dyn->d_tag < DT_NUM)
            l_info[dyn->d_tag] = dyn;
    }
}

/* Deal whith name */
static void _fill_module_name(const char * l_name, umain_elf_t * elf)
{
    if (l_name[0] == '\0') dasics_strcpy(elf->l_name, *(char**)(user_sp + 8));
    else dasics_strcpy(elf->l_name, l_name);

    int length = dasics_strlen(elf->l_name);
    char * real_name = elf->l_name;
    for (int i = 0; i < length ; i++)
    {
        if (elf->l_name[i] == '/') real_name = &(elf->l_name[i + 1]);
    }
    
    elf->real_name = real_name;

}

/*
 * This function is used for no lazy dynamic call
 * Found all the target function addr
 */
static void _fill_local_got(umain_elf_t * elf)
{
    elf->plt_begin = (uint64_t *)elf->got_begin[3];
    elf->_plt_start = (uint64_t)elf->plt_begin + 0x20;
    elf->_plt_end = elf->_plt_start + 0x10 * elf->got_num;  

    for (int i = 2; i < elf->got_num + 2; i++)
    {
        /*
        * We found that the Plt[x] wants to use dely binding to find the fucntion,
        * and we prepare all the parameters, and jump
        * 
        * dll_a0: the got[1], struct link_map of the library
        * dll_a1: the thrice of the plt table offset
        * ulib_func: the addr of the ulib function 
        */

        uint64_t dll_a0 = (uint64_t)elf->map_link;
        uint64_t dll_a1 = (((uint64_t)(i - 2) * 0x10UL) >> 1) * 3;
        uint64_t ulib_func = elf->fixup_handler(dll_a0, dll_a1);   

        elf->_local_got_table[i] = ulib_func;
        // Recover the plt begin
            // Only elf needed
    down:
        elf->got_begin[i] = (uint64_t)elf->plt_begin;     
    }


    uint64_t start = ROUNDDOWN(elf->l_relro_addr, PAGE_SIZE);
    uint64_t end = ROUND(elf->l_relro_addr + elf->l_relro_size, PAGE_SIZE);
    
    _dasics_mprotect((void *)start, \
                        end - start, \
                        PROT_READ | PROT_WRITE);
    elf->got_begin[0] = (uint64_t)dasics_umaincall;
    elf->got_begin[1] = (uint64_t)elf;  
    _dasics_mprotect((void *)start, \
                        end - start, \
                        PROT_READ);      

}

/*
 * According to the binary to scan the phdr
 *  get all the _*_start, _*_enf
 */
void _fill_module_map(umain_elf_t * elf)
{
    // binary image
    ElfW(Phdr) * _Phdr;
    ElfW(Half) Phnum;

    if (!elf->l_addr)
    {
        elf->l_phdr = (ElfW(Phdr) *)_get_auxv_entry(user_sp, AT_PHDR);
        elf->l_phnum = (ElfW(Half))_get_auxv_entry(user_sp, AT_PHNUM);
        goto setup;
    } 
    
    char * binary = (char *)elf->l_addr;

    if (!is_elf_format(binary)) return;

    elf->l_ehdr = (ElfW(Ehdr) *)binary;
    elf->l_phdr = (ElfW(Phdr) *)(elf->l_addr + elf->l_ehdr->e_phoff);
    elf->l_phnum = elf->l_ehdr->e_phnum;

setup:
    _Phdr = (ElfW(Phdr) *)elf->l_phdr;
    Phnum = elf->l_phnum;

    int _set_r = 0;
    int _set_w = 0;
    int _set_x = 0;    
    int _set_map = 0;
    elf->_r_start = elf->_r_end = 0;
    elf->_w_start = elf->_w_end = 0;

    for (int _i = 0; _i < Phnum; _i++)
    {
        #define ADD_TWO(X, Y)((X) + (Y))
        #define ADD_THREE(X, Y, Z)((X) + (Y) + (Z))

        switch (_Phdr[_i].p_type)
        {
        case PT_LOAD:
            /* code */
                        // Only set for the executable
            if (!_set_map)
            {
                elf->_map_start = ADD_TWO(_Phdr[_i].p_vaddr, \
                                                        elf->l_addr);
                elf->_map_end = elf->_map_start;
            }

            if (_set_map) elf->_map_end = ROUND(ADD_THREE(_Phdr[_i].p_vaddr,  \
                                                _Phdr[_i].p_memsz, \
                                                elf->l_addr), PAGE_SIZE);

            if (!_set_x && \
                (_Phdr[_i].p_flags & PF_X))
            {
                _set_x = 1;
                elf->_text_start = ADD_TWO(_Phdr[_i].p_vaddr, \
                                                        elf->l_addr);
            }

            if ((_Phdr[_i].p_flags & PF_X))
            {
                elf->_text_end = ADD_THREE(_Phdr[_i].p_vaddr,  \
                                                _Phdr[_i].p_memsz, \
                                                elf->l_addr);
            }                

            // Only set for the READ_ONLY type Phdr 
            if (!_set_r && \
                (_Phdr[_i].p_flags & PF_R) && \
                !(_Phdr[_i].p_flags & PF_W)
                )
            {
                _set_r = 1;
                elf->_r_start = ADD_TWO(_Phdr[_i].p_vaddr, \
                                                        elf->l_addr);
            } 
                
            if ((_Phdr[_i].p_flags & PF_R) && \
                !(_Phdr[_i].p_flags & PF_W)
                )
            {
                elf->_r_end = ADD_THREE(_Phdr[_i].p_vaddr,  \
                                                _Phdr[_i].p_memsz, \
                                                elf->l_addr);
            } 

            // Only set for the WRITE_ONLY type Phdr
            if (!_set_w && \
                (_Phdr[_i].p_flags & PF_R) && \
                (_Phdr[_i].p_flags & PF_W)
                )
            {
                _set_w = 1;
                elf->_w_start = ADD_TWO(_Phdr[_i].p_vaddr, \
                                                        elf->l_addr);
            } 
            
            if ((_Phdr[_i].p_flags & PF_R) && \
                (_Phdr[_i].p_flags & PF_W)
                )
            {
                elf->_w_end = ADD_THREE(_Phdr[_i].p_vaddr,  \
                                                _Phdr[_i].p_memsz, \
                                                elf->l_addr);
            } 
            break;

        case PT_GNU_RELRO:
            elf->l_relro_addr = _Phdr[_i].p_vaddr + elf->l_addr;  
            elf->l_relro_size = _Phdr[_i].p_memsz;
            break;

        default:
            break;
        }
    }
}

static void _find_copy_lib(umain_elf_t * elf)
{
    umain_elf_t * tmp = _umain_elf_table; 
    
    if (tmp == NULL) 
    {
        dasics_printf("[DASICS_ERROR]: Error in _find_copy_lib, copy lib not found\n");
        while(1);        
    }

    do {
        if ((tmp->_flags & MAIN_AREA) && \
            !dasics_strcmp(tmp->l_name, elf->l_name))
        {
            tmp->_copy_lib_elf = elf;
            elf->_copy_lib_elf = tmp;
            return;
        }
        tmp = tmp->umain_elf_next;
    } while(tmp != _umain_elf_table);

    dasics_printf("[DASICS_ERROR]: Error in _find_copy_lib, copy lib not found\n");
    while(1);      
}

/* Fill target elf addr */
static void _fill_target_elf()
{
    umain_elf_t * tmp_elf = _umain_elf_table;

    do
    {
        /* code */
        for (int i = 0; i < tmp_elf->got_num; i++)
        {
            /* code */
            tmp_elf->target_elf[i + 2] = _get_area(tmp_elf->_local_got_table[i + 2]);
            tmp_elf->target_func_name[i + 2] = _get_lib_name(tmp_elf, i);
            // if (tmp_elf == _umain_elf_table && (tmp_elf->target_elf[i + 2]->_flags & MAIN_AREA))
            // {
            //     tmp_elf->got_begin[i + 2] = tmp_elf->_local_got_table[i+2];
            // }

        }
        tmp_elf = tmp_elf->umain_elf_next;
    } while (tmp_elf != _umain_elf_table);

}

/*
 * This fuinction is used to init umain dasics_link_map
 * include the exe, linker, all library
 */
int create_umain_elf_chain(struct link_map * main_elf)
{
    struct link_map * _map_init = main_elf;

    // Judge trust area
    extern uint64_t _start[];
    // Build all chain
    while (_map_init != NULL)
    {
        /* Dasics Copy stage don't do main elf again */
        if (dasics_stage == DASICS_COPY_LIB && !_map_init->l_addr)
        {
            // Correct the main
            _fill_local_got(_umain_elf_table);
            goto jump;
        }

        // DASICS_COPY_STAGE will use stdlib and avoid to change BRK
        umain_elf_t * _elf = dasics_stage == DASICS_COPY_LIB ? (umain_elf_t *)malloc(sizeof(umain_elf_t)) :
                             (umain_elf_t *)dasics_malloc(sizeof(umain_elf_t));

        dasics_memset(_elf, 0, sizeof(umain_elf_t));

        if (_elf == NULL)
        {
            dasics_printf("[DASICS ERROR]: dasics_malloc error\n");
            while(1);
        }

        _fill_dynamic_section(_elf->l_info, _map_init->l_ld);
        _fill_module_name(_map_init->l_name, _elf);

        // dasics_printf("[LOG]: DASICS module:%s\n", _elf->real_name);
        _elf->fixup_handler = dasics_stage == DASICS_COPY_LIB ? dll_fixup_handler_lib : 
                                                dll_fixup_handler;
        _elf->map_link = _map_init;
        _elf->_copy_lib_elf = NULL;
        _elf->dynamic = (uint64_t)_map_init->l_ld;
        _elf->l_addr = _map_init->l_addr;  

        // the flag
        ElfW(Addr) map_base = _elf->l_addr; 
        if (map_base < (uint64_t)_start) _elf->_flags |= MAIN_AREA;
        else 
        {
            _elf->_flags |= LIB_AREA;
            // Find copy lib
            if (dasics_stage == 2)
            {
                _find_copy_lib(_elf);
            }
        }

        if (!map_base)  _elf->_flags |= ELF_AREA;
        if (map_base && _get_auxv_entry(user_sp ,AT_DASICS))
            if (!dasics_strcmp(_map_init->l_name, _interp_start))
                _elf->_flags |= LINK_AREA;
        
        
        // no got table
        if (_elf->l_info[DT_PLTGOT] == NULL) goto no_pltgot;    
        /* Init local got table for module */
        _elf->got_begin = (uint64_t *)(_elf->l_info[DT_PLTGOT]->d_un.d_val + \
                                            _map_init->l_addr);
        
        init_list(&_elf->func_man);

        uint64_t judge_dynamic = _elf->dynamic - _elf->l_addr;

        // Calculate the number of got table
        int rela_got_num = 0;

        // rela.plt only used for func, but .rela.dyn used for variable
        rela_got_num = _elf->l_info[DT_PLTRELSZ]->d_un.d_val / sizeof(Elf64_Rela);

        

        // no got table
        if (rela_got_num == 0) goto no_pltgot;

        _elf->got_num = rela_got_num;
        uint64_t alloc_num = rela_got_num + 2;
        _elf->_local_got_table = (uint64_t *)dasics_malloc(alloc_num * sizeof(uint64_t) );
        _elf->local_func = (struct func_mem **)dasics_malloc(alloc_num * sizeof(struct func_mem *));
        _elf->redirect_switch = (int *)dasics_malloc(alloc_num * sizeof(int));
        _elf->target_elf = (umain_elf_t **)dasics_malloc(alloc_num* sizeof(umain_elf_t *));
        _elf->_local_call_time = (uint64_t *)dasics_malloc(alloc_num * sizeof(uint64_t) );
        _elf->target_func_name = (char **)dasics_malloc(alloc_num * sizeof(char *));
        // Now, we will calculate all JMPREL's value，and copy them to the local got table
        _elf->calculate = 0;

        // fill the *_start, *_end of 
        _fill_module_map(_elf);


        if (_elf->_local_got_table == NULL || \
                _elf->local_func == NULL || \
                    _elf->redirect_switch == NULL || \
                        _elf->target_elf == NULL)
        {
            dasics_printf("[DASICS ERROR]: dasics_malloc error\n");
            while(1);
        } 

        dasics_memset(_elf->_local_got_table, 0, alloc_num * sizeof(uint64_t));
        dasics_memset(_elf->local_func, 0, alloc_num * sizeof(struct func_mem *));
        dasics_memset(_elf->redirect_switch, 0, alloc_num * sizeof(int));
        dasics_memset(_elf->target_elf, 0, alloc_num * sizeof(umain_elf_t *));
        dasics_memset(_elf->_local_call_time, 0, alloc_num * sizeof(uint64_t));
        dasics_memset(_elf->target_func_name, 0, alloc_num * sizeof(char *));


        if (_umain_elf_table == NULL)
            dasics_main_elf = _elf;

        // Linker not do
        if (dasics_strcmp(_elf->l_name, _interp_start))
            _fill_local_got(_elf);

no_pltgot:
        // Add into chain
        if (_umain_elf_table == NULL) 
        {
            _umain_elf_table = _elf;
            _umain_elf_table->umain_elf_next = \
                _umain_elf_table->umain_elf_prev = \
                    _umain_elf_table;
        }
        else 
        {
            _elf->umain_elf_next = _umain_elf_table;
            _elf->umain_elf_prev = _umain_elf_table->umain_elf_prev;
            _umain_elf_table->umain_elf_prev->umain_elf_next = _elf;  
            _umain_elf_table->umain_elf_prev = _elf;
        }

    jump:
        _map_init = _map_init->l_next;

    }
    
    // fill target elf addr
    _fill_target_elf();

    
    return 0;

}

void open_memory(umain_elf_t * _main)
{
    ElfW(Phdr) * _Phdr = (ElfW(Phdr) *)_main->l_phdr;
    // Circulate the PT_LOAD Phdr
    for (int _i = 0; _i < _main->l_phnum; _i++)
    {
        if (_Phdr[_i].p_type == PT_LOAD)
            if (_Phdr[_i].p_flags & PF_W)
            {
                _dasics_mprotect((void *)ROUNDDOWN(_Phdr[_i].p_vaddr, PAGE_SIZE), 
                        ROUND(_Phdr[_i].p_vaddr + _Phdr[_i].p_memsz, PAGE_SIZE) \
                        - ROUNDDOWN(_Phdr[_i].p_vaddr, PAGE_SIZE),
                        PROT_READ | PROT_WRITE
                        );
            }
    }    


}

struct link_map * get_main_link()
{
    // get the struct link_map
    Elf64_Dyn * dyn =  NULL;
    struct r_debug * debug_extended =  NULL;
#ifdef DYNAMIC
    for(dyn = _DYNAMIC; dyn->d_tag != DT_NULL; ++dyn)
    {
        if(dyn->d_tag == DT_DEBUG)
        {
            debug_extended = (struct r_debug *)dyn->d_un.d_ptr;
        }          
    }
#endif
    struct link_map * link = NULL;

    if (debug_extended == NULL)
    {
        extern uint64_t _got_start[];
        link = (struct link_map *)_got_start[1];
    } else 
    {
        link = debug_extended->r_map;
    }    

    return link;
}

void init_elf_plt(umain_elf_t * elf, uint32_t * pltPc, uint64_t * gotAddr)
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

#define IMM_LD         0xfff00000UL
#define IMM_AUIPC      0xfffff000UL

    int64_t auipc_imm = (int64_t)(auipc & IMM_AUIPC);

    uint64_t t3 = (uint64_t)pltPc + auipc_imm;
    uint64_t ld_imm = (uint64_t)((uint64_t)((ld & IMM_LD)>> 20));

    int64_t ld_offset = 0;

#define NEGATIVE_FLAG 0x00000800
#define NEGATIVE      0xfffffffffffff000
    if (ld_imm & NEGATIVE_FLAG)
        ld_offset = (int64_t)(NEGATIVE | ld_imm);
    else 
        ld_offset = ld_imm;
    // This load addr of t3
    uint64_t addr = t3 + ld_offset;

    elf->plt_begin = (uint64_t *)((uint64_t)pltPc - 0x10 * ((addr - (uint64_t)gotAddr) / 8));
    elf->_plt_start = (uint64_t)elf->plt_begin + 0x20;
                        
    elf->_plt_end = elf->_plt_start + 0x10 * elf->got_num;    

}

