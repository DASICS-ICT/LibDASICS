#include <stdlib.h>
#include <utrap.h>
#include <stdio.h>
#include <dasics_stdio.h>

/*
 * This function is used to deal with the DasicsULoadFault
 * error return -1
 */
int handle_DasicsULoadFault(struct ucontext_trap * regs)
{
    int idx = 0;
    uint16_t c_ld;
    uint32_t _ld;

//     umain_got_t * _got_entry = _get_area(regs->uepc);
// #ifdef DASICS_DEBUG
//     if (_got_entry)
//         dasics_printf("[ufault info]: hit read ufault uepc: 0x%lx, address: 0x%lx \n", regs->uepc, regs->utval);
//     // debug_print_umain_map(_got_entry);
// #endif
//     if (_got_entry == NULL)
//     {
//     #ifdef DASICS_DEBUG
//         dasics_printf("[error]: failed to find the _got_entry\n");
//     #endif
//         if (_umain_got_table == NULL) goto dasics_static_load;
//         exit(1);
//     }
    dasics_printf("[DASICS_EXCEPTION]: Load fault: 0x%lx, pc: 0x%lx\n", regs->utval, regs->uepc);


dasics_static_load:
    

    #define LD_INCMASK 0x00000003

    // Jump load instruction for future excution
    c_ld = *((uint16_t *)regs->uepc);
    _ld = *((uint32_t *)regs->uepc);

    if ((_ld & LD_INCMASK) == LD_INCMASK)
        regs->uepc += 4;
    else 
        regs->uepc += 2;

    
    if (idx == -1)
    {
        dasics_printf("[error]: no more libbounds!!\n");
        exit(1);
    }    
    return 0;
}


/*
 * This function is used to deal with the DasicsULoadFault  
 * error return -1
 */
int handle_DasicsUStoreFault(struct ucontext_trap * regs)
{
    int idx = 0;
    uint16_t c_sd;
    uint32_t _sd;
//     umain_got_t * _got_entry = _get_area(regs->uepc);
// // #ifdef DASICS_DEBUG
//     dasics_printf("[ufault info]: hit write ufault uepc: 0x%lx, address: 0x%lx \n", regs->uepc, regs->utval);
//     // debug_print_umain_map(_got_entry);
// // #endif
//     if (_got_entry == NULL)
//     {
//     #ifdef DASICS_DEBUG
//         dasics_printf("[error]: failed to find the _got_entry\n");
//     #endif
//         if (_umain_got_table == NULL) goto dasics_static_store;
//         exit(1);
//     }
    dasics_printf("[DASICS_EXCEPTION]: Store fault: 0x%lx pc: 0x%lx\n", regs->utval, regs->uepc);

dasics_static_store:
    #define SD_INCMASK 0x00000003

    // Jump load instruction for future excution
    c_sd = *((uint16_t *)regs->uepc);
    _sd = *((uint32_t *)regs->uepc);

    if ((_sd & SD_INCMASK) == SD_INCMASK)
        regs->uepc += 4;
    else 
        regs->uepc += 2;


    if (idx == -1)
    {
        dasics_printf("[error]: no more libbounds!!\n");
        exit(1);
    }    
    return 0;
}
 