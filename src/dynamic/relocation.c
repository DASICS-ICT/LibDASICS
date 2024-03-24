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
    if (find_item(func_name) != NULL)
    {
        dasics_printf("[Warning]: There has exited one redirect item: %s\n", func_name);
        return 0;
    }

    // Malloc one
    redirect_t * new_item = (redirect_t *)malloc(sizeof(redirect_t));

    dasics_strcpy(new_item->name, func_name);


    list_add_tail(&new_item->list, &redirect_table);

    return 0;
}

/* Delete one item from list */ 
int delete_redirect_item(const char *func_name)
{
    redirect_t *delete_item = NULL;

    // Find one item
    if ((delete_item = find_item(func_name)) == NULL)
    {
        dasics_printf("[Warning]: There has no one redirect item: %s\n", func_name);
        return 0;
    }


    list_del(&delete_item->list);

    free(delete_item);

    return 0;
}

/* Force relocation */
uint64_t force_redirect(umain_elf_t * entry, const char * func_name, uint64_t target)
{
    if (!redirect_switch) return target;

    // Never redirect __libc_start_main
    if (!dasics_strcmp(func_name, "__libc_start_main"))
    {
        return target;
    }


    // If force_redirect_switch open, force redirect
    if (find_item(func_name) == NULL && !force_redirect_switch) return target;
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


int force_redirect_open()
{
    force_redirect_switch = 1;
    return 0;
}

int force_redirect_close()
{
    force_redirect_switch = 0;
    return 0;
}