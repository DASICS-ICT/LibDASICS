#include <utrap.h>
#include <dasics_stdio.h>

/*
 * This function is used to deal with the DasicsULoadFault
 * error return -1
 */
int handle_DasicsULoadFault(struct ucontext_trap * r_regs)
{
    int idx = 0;
//     umain_got_t * _got_entry = _get_trap_area(r_regs->uepc);
// #ifdef DASICS_DEBUG
//     if (_got_entry)
//         dasics_printf("[ufault info]: hit read ufault uepc: 0x%lx, address: 0x%lx \n", r_regs->uepc, r_regs->utval);
//     // debug_print_umain_map(_got_entry);
// #endif
//     if (_got_entry == NULL)
//     {
//     #ifdef DASICS_DEBUG
//         my_printf("[error]: failed to find the _got_entry\n");
//     #endif
//         if (_umain_got_table == NULL) goto dasics_static_load;
//         exit(1);
//     }

dasics_static_load:
    

    #define CLD_INCMASK 0xe003

    #define CLD_MASK    0x6000
    #define CLDSP_MASK  0x6002
    #define CLW_MASK    0x4000
    #define CLWSP_MASK  0x4002

    // Jump load instruction
    uint16_t c_ld = *((uint16_t *)r_regs->uepc);
    if ((c_ld & CLD_INCMASK) == CLD_MASK || \
        (c_ld & CLD_INCMASK) == CLDSP_MASK || \
        (c_ld & CLD_INCMASK) == CLW_MASK || \
        (c_ld & CLD_INCMASK) == CLWSP_MASK)
        r_regs->uepc += 2;
    else 
        r_regs->uepc += 4;

    
    if (idx == -1)
    {
        my_printf("[error]: no more libbounds!!\n");
        // while(1);
        exit(1);
    }    
    return 0;
}


/*
 * This function is used to deal with the DasicsULoadFault  
 * error return -1
 */
int handle_DasicsUStoreFault(struct ucontext_trap * r_regs)
{
    int idx = 0;
//     umain_got_t * _got_entry = _get_trap_area(r_regs->uepc);
// // #ifdef DASICS_DEBUG
//     dasics_printf("[ufault info]: hit write ufault uepc: 0x%lx, address: 0x%lx \n", r_regs->uepc, r_regs->utval);
//     // debug_print_umain_map(_got_entry);
// // #endif
//     if (_got_entry == NULL)
//     {
//     #ifdef DASICS_DEBUG
//         my_printf("[error]: failed to find the _got_entry\n");
//     #endif
//         if (_umain_got_table == NULL) goto dasics_static_store;
//         exit(1);
//     }

dasics_static_store:
    #define CSD_INCMASK 0xe003

    #define CSD_MASK    0xe000
    #define CSDSP_MASK  0xe002
    #define CSW_MASK    0xc000
    #define CSWSP_MASK  0xc002

    // Jump store instruction
    uint16_t c_sd = *((uint16_t *)r_regs->uepc);
    if ((c_sd & CSD_INCMASK) == CSD_MASK || \
        (c_sd & CSD_INCMASK) == CSDSP_MASK || \
        (c_sd & CSD_INCMASK) == CSW_MASK || \
        (c_sd & CSD_INCMASK) == CSWSP_MASK)
        r_regs->uepc += 2;
    else 
        r_regs->uepc += 4;
    if (idx == -1)
    {
        my_printf("[error]: no more libbounds!!\n");
        exit(1);
    }    
    return 0;
}
 