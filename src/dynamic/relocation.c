#include <dynamic.h>
#include <udirect.h>
#include <stdlib.h>
#include <dasics_start.h>
#include <dasics_string.h>
#include <dasics_stdio.h>


// List
LIST_HEAD(redirect_table);

// switch
int redirect_switch = 0;
int force_redirect_switch = 0;

static int _find_idx_by_name(umain_elf_t * target, const char *name)
{
    for (int  i = 0; i < target->got_num - 2; i++)
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


/* Add one item  */
int add_redirect_item(const char *func_name)
{
    // if (find_item(func_name) != NULL)
    // {
    //     dasics_printf("[Warning]: There has exited one redirect item: %s\n", func_name);
    //     return 0;
    // }

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
    redirect_switch = 1;
    return 0;
}

// Close the switch
int close_redirect()
{
    redirect_switch = 0;
    return 0;
}
