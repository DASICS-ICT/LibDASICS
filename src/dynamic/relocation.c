#include <dynamic.h>
#include <udirect.h>
#include <stdlib.h>
#include <dasics_start.h>
#include <dasics_string.h>
#include <dasics_stdio.h>
#include <dmalloc.h>
#include <udasics.h>


// List
LIST_HEAD(redirect_table);

// switch
int redirect_switch = 0;
int force_redirect_switch = 0;

// white_list
char * white_list[]= {
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
    "__libc_start_main",
    "free",
    "malloc",
    "realloc",
    "srand",
    "calloc",
    "fprintf",
    "ungetc",
    "getenv",
    "fwrite",
    "fopen64",
    "fputs",
    "fputc",
    // "vsprintf",
    "read",
    "fread",
    "putc",
    "getc",
    // "strerror",
    "close",
    "puts",
    "getcwd",
    "printf",
    "vfprintf",
    "fclose",
    "open64",
    "feof",
    "fflush",
    "ferror",
    "fstat64",
    "__assert_fail",
    "obstack_free",
    "_obstack_memory_used",
    "_obstack_newchunk",
    "_obstack_begin",
    "signal",
    "_setjmp",
    "setjmp",
    "longjmp",
    NULL
};

static int _find_idx_by_name(umain_elf_t * target, const char *name)
{
    for (int  i = 0; i < target->got_num; i++)
    {
        if (!dasics_strcmp(_get_lib_name(target, i), name))
        {
            return i;
        }
    }

    return -1;
}


/* This func is used to reloc the target */
uint64_t _call_reloc(umain_elf_t *elf, uint64_t target)
{
    if (_umain_elf_table == NULL) return 0;
    /* Get the target area of the target addr */
    umain_elf_t * _target_got = _get_area(target);
    if (_target_got == NULL)
    {
        dasics_printf("[ERROR] DASICS error! target addr error: 0x%lx!\n", target);
        exit(1);
    }

    /* The trusted area want to call trusted func */
    if ((elf->_flags & MAIN_AREA) &&
         (_target_got->_flags & MAIN_AREA))
    {
        return target;
    }
        
    /* The trusted area want to call untrusted func */
    if ((elf->_flags & MAIN_AREA) &&
         !(_target_got->_flags & MAIN_AREA))
    {
        if (_target_got->_copy_lib_elf != NULL)
        {
            return (target - _target_got->l_addr) + _target_got->_copy_lib_elf->l_addr;
        }
    }

    /* The untrusted area want to call trusted func */
    if (!(elf->_flags & MAIN_AREA) &&
         (_target_got->_flags & MAIN_AREA))
    {
        if (_target_got->_copy_lib_elf != NULL)
        {
            return (target - _target_got->l_addr) +  _target_got->_copy_lib_elf->l_addr;
        }
    }    

    return target;    

}

/**
 * @brief Find one item from the list 
 */
static redirect_t * find_item(const char *name)
{
    redirect_t * item_entry = NULL;
    redirect_t * item_q = NULL;

    list_for_each_entry_safe(item_entry, item_q ,&redirect_table, list)
    {
        if (!dasics_strcmp(name, item_entry->name))
        {
            return item_entry;
        }
    }
    return NULL;
}

void set_trampoline()
{
    if (_umain_elf_table == NULL) return;

    umain_elf_t *elf = _umain_elf_table;

    // force all func redirect
    for (int i = 0; i < elf->got_num; i++) {
        elf->redirect_switch[i + 2] = REDIRECT;
        elf->got_begin[i + 2] = elf->_plt_begin;
    }
    
    // make white list func direct
    for (int i = 0; white_list[i]; i++) {
        char *func_name = white_list[i];

        int idx = _find_idx_by_name(elf, func_name);

        if (idx == -1) continue;

        elf->redirect_switch[idx + 2] = DIRECT;
        elf->got_begin[idx + 2] = elf->_local_got_table[idx + 2];
    }
}

void print_trampoline() 
{
    if (_umain_elf_table == NULL) return;

    umain_elf_t *elf = _umain_elf_table;

    dasics_printf("[LOG]: untrusted func:\n");
    for (int i = 0; i < elf->got_num; i++) {
        if (elf->redirect_switch[i + 2] == REDIRECT) {
            dasics_printf("[LOG] \t\t%s(%s)\n", 
                elf->target_elf[i + 2]->real_name, 
                elf->target_func_name[i + 2]);
        }
    }
    dasics_printf("[LOG]: trusted func:\n");
    for (int i = 0; i < elf->got_num; i++) {
        if (elf->redirect_switch[i + 2] == DIRECT) {
            dasics_printf("[LOG] \t\t%s(%s)\n", 
                elf->target_elf[i + 2]->real_name, 
                elf->target_func_name[i + 2]);
        }
    }
}

void print_chain_info() 
{
    umain_elf_t * tmp_elf = _umain_elf_table;
    if (tmp_elf == NULL) return;
    do {
        dasics_printf("[LOG]: %s, plt_begin:0x%lx\n", tmp_elf->real_name, tmp_elf->plt_begin);
        for (int i = 2; i < tmp_elf->got_num + 2; i++) {
            dasics_printf("[LOG]: \t\t(%s), got_addr:0x%lx, lib_addr:0x%lx\n", 
                tmp_elf->target_func_name[i], 
                tmp_elf->got_begin[i],
                tmp_elf->_local_got_table[i]);
        }
        tmp_elf = tmp_elf->umain_elf_next;
    } while (tmp_elf != _umain_elf_table);
}


/* Add one item  */
int add_redirect_item(const char *func_name)
{
    // if (find_item(func_name) != NULL)
    // {
    //     dasics_printf("[Warning]: There has exited one redirect item: %s\n", func_name);
    //     return 0;
    // }
    if (_umain_elf_table == NULL) return 0;

    // Never redirect __libc_start_main
    if (!dasics_strcmp(func_name, "__libc_start_main"))
    {
        return 0;
    }

    int idx = _find_idx_by_name(_umain_elf_table, func_name);

    if (idx == -1)
    {
        dasics_printf("[Warning]: Not function: %s in this module\n", func_name);
        return 0;
    }
    // Open switch
    _umain_elf_table->redirect_switch[idx + 2] = 1;

    umain_elf_t * target = _umain_elf_table->target_elf[idx+2];


    return 0;
}

/* Delete one item from list */ 
int delete_redirect_item(const char *func_name)
{
    if (_umain_elf_table == NULL) return 0;

    int idx = _find_idx_by_name(_umain_elf_table, func_name);

    if (idx == -1)
    {
        dasics_printf("[Warning]: Not function: %s in this module\n", func_name);
        return 0;
    }
    // Close switch and switch func
    _umain_elf_table->redirect_switch[idx + 2] = 0;
    umain_elf_t * target = _umain_elf_table->target_elf[idx+2];


    return 0;
}

/* Force relocation */
uint64_t force_redirect(umain_elf_t * entry, int idx, uint64_t target)
{
    if (_umain_elf_table == NULL) return 0;

    if (!redirect_switch) return target;


    if (idx == -1) return target;
    if (entry->redirect_switch[idx + 2] == 0) return target;

    /* Get the target area of the target addr */
    umain_elf_t * _target_got = _get_area(target);   

    if (_target_got == NULL)
    {
        dasics_printf("[ERROR] DASICS error! target addr error: 0x%lx!\n", target);
        exit(1);
    }
    // get one redirect address
    /* The trusted area want to call trusted func */
    if ((entry->_flags & MAIN_AREA) &&
         (_target_got->_flags & MAIN_AREA))
    {
        if (_target_got->_copy_lib_elf != NULL)
        {
            entry->target_elf[idx + 2] = _target_got->_copy_lib_elf;
            return (target - _target_got->l_addr) + _target_got->_copy_lib_elf->l_addr;
        }
    }


    return target;
}

// Open teh switch
int open_redirect()
{
    if (_umain_elf_table == NULL) return 0;

    redirect_switch = 1;
    return 0;
}

// Close the switch
int close_redirect()
{
    if (_umain_elf_table == NULL) return 0;

    redirect_switch = 0;
    return 0;
}
