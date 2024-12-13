#include <dynamic.h>
#include <assert.h>
#include <spec.h>

// Add new ignore function here
char * ignore_func[] = 
{
    "memcpy",
    "memmove",
    "memcmp",
    "memset",
    "strlen",
    "strcpy",
    "strcmp",
    "strncpy",
    "strncmp",
    "__sigsetjmp",
    "siglongjmp",
    "strchr",
    "strtol",
    NULL
};

void ignore_simple_function()
{

    umain_elf_t * target = _umain_elf_table;
    // for (int j = 0; j < target->got_num; j++)
    // {
    //     dasics_printf("Function %s\n", _get_lib_name(target, j));
    // }
    for (int i = 0; ignore_func[i]; i++)
    {
        char * str_func = ignore_func[i];


        for (int j = 0; j < target->got_num; j++)
        {
            char * target_str_func = _get_lib_name(target, j);
            if (!dasics_strcmp(target_str_func, str_func))
            {
                umain_elf_t * func_elf =  target->target_elf[j + 2];
                assert(!dasics_strcmp(target_str_func, target->target_func_name[j+2]));
                assert(func_elf->_copy_lib_elf != NULL);
                dasics_printf("> [DASICS SPEC]: Ignore funcion: %s\n", str_func);
                uint64_t prev_addr = target->_local_got_table[j + 2];
                uint64_t reloc_addr = 0;
                // reloc_addr
                if (func_elf->l_addr > func_elf->_copy_lib_elf->l_addr)
                {
                    reloc_addr = prev_addr - (func_elf->l_addr - func_elf->_copy_lib_elf->l_addr);
                }
                else 
                {
                    reloc_addr = prev_addr + (func_elf->_copy_lib_elf->l_addr, func_elf->l_addr);
                }
                // reloc addr
                target->got_begin[j + 2] = reloc_addr;
                break;
            }
        }

    }
    return;
}