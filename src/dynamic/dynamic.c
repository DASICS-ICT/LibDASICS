#include <elf.h>
#include <link.h>
#include <stddef.h>

#include <list.h>
#include <dynamic.h>
#include <dasics_string.h>
#include <dasics_stdio.h>
#include <dasics_start.h>

umain_elf_t * _umain_elf_table = NULL;
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
 * According to the binary to scan the phdr
 *  get all the _*_start, _*_enf
 */
void _fill_module_map(umain_elf_t * elf)
{
    // binary image

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
    ElfW(Phdr) * _Phdr = (ElfW(Phdr) *)elf->l_phdr;
    ElfW(Half) Phnum = elf->l_phnum;

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
        if (_Phdr[_i].p_type == PT_LOAD)
        {
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
        if (dasics_stage == 2 && !_map_init->l_addr) goto jump;

        umain_elf_t * _elf = (umain_elf_t *)dasics_malloc(sizeof(umain_elf_t));

        dasics_memset(_elf, 0, sizeof(umain_elf_t));

        if (_elf == NULL)
        {
            dasics_printf("[DASICS ERROR]: dasics_malloc error\n");
            while(1);
        }

        _fill_dynamic_section(_elf->l_info, _map_init->l_ld);
        _fill_module_name(_map_init->l_name, _elf);

        dasics_printf("[LOG]: DASICS module:%s\n", _elf->real_name);
        _elf->fixup_handler = dasics_stage == 2 ? dll_fixup_handler_lib : dll_fixup_handler;
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
        for (rela_got_num  = 0; \
                _elf->got_begin[rela_got_num] != judge_dynamic;\
                    rela_got_num++);
        // no got table
        if (rela_got_num == 0) goto no_pltgot;

        _elf->got_num = rela_got_num;
        _elf->_local_got_table = (uint64_t *)dasics_malloc(rela_got_num * sizeof(uint64_t));
        _elf->local_func = (struct func_mem **)dasics_malloc(rela_got_num * sizeof(struct func_mem *));

        // fill the *_start, *_end of 
        _fill_module_map(_elf);


        if (_elf->_local_got_table == NULL)
        {
            dasics_printf("[DASICS ERROR]: dasics_malloc error\n");
            while(1);
        }

        dasics_memset(_elf->_local_got_table, 0, rela_got_num * sizeof(uint64_t));
        
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
    
    
    return 0;

}

#include <sys/mman.h>
void open_memory(umain_elf_t * _main)
{
    ElfW(Phdr) * _Phdr = (ElfW(Phdr) *)_main->l_phdr;
    // Circulate the PT_LOAD Phdr
    for (int _i = 0; _i < _main->l_phnum; _i++)
    {
        if (_Phdr[_i].p_type == PT_LOAD)
            if (_Phdr[_i].p_flags & PF_W)
            {
                mprotect((void *)ROUNDDOWN(_Phdr[_i].p_vaddr, PAGE_SIZE), 
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
    for(dyn = _DYNAMIC; dyn->d_tag != DT_NULL; ++dyn)
    {
        if(dyn->d_tag == DT_DEBUG)
        {
            debug_extended = (struct r_debug *)dyn->d_un.d_ptr;
        }          
    }
    
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

