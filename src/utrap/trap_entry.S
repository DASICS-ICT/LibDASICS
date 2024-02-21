#include <asm/asm.h>
#include <ucsr.h>
#include <asm/offset.h>

.macro SAVE_CONTEXT
    addi sp, sp, -(OFFSET_SIZE) 
    /* save all general purpose registers here! */
    // return address
    sd ra, OFFSET_TRAP_RA(sp)

    // pointers, sp and tp will be handled later

    // temporary
    sd t0, OFFSET_TRAP_T0(sp)
    sd t1, OFFSET_TRAP_T1(sp)
    sd t2, OFFSET_TRAP_T2(sp)

    mv t0, sp 
    addi t0, t0, OFFSET_SIZE
    sd t0, OFFSET_TRAP_SP(sp)

    // args
    sd a0, OFFSET_TRAP_A0(sp)
    sd a1, OFFSET_TRAP_A1(sp)
    sd a2, OFFSET_TRAP_A2(sp)
    sd a3, OFFSET_TRAP_A3(sp)
    sd a4, OFFSET_TRAP_A4(sp)
    sd a5, OFFSET_TRAP_A5(sp)
    sd a6, OFFSET_TRAP_A6(sp)
    sd a7, OFFSET_TRAP_A7(sp)

    // temporary register
    sd t3, OFFSET_TRAP_T3(sp)
    sd t4, OFFSET_TRAP_T4(sp)
    sd t5, OFFSET_TRAP_T5(sp)
    sd t6, OFFSET_TRAP_T6(sp)    

    csrr t0, CSR_UEPC
    sd t0, OFFSET_TRAP_UEPC(sp)

    csrr t0, CSR_UCAUSE
    sd t0, OFFSET_TRAP_UCAUSE(sp)

    csrr t0, CSR_UTVAL
    sd t0, OFFSET_TRAP_UTVAL(sp)

.endm

.macro RESTORE_CONTEXT
    ld t0, OFFSET_TRAP_UEPC(sp)
    csrw CSR_UEPC, t0 

    ld t0, OFFSET_TRAP_UCAUSE(sp)
    csrw CSR_UCAUSE, t0 

    ld t0, OFFSET_TRAP_UTVAL(sp)
    csrw CSR_UTVAL, t0 


    // return address
    ld ra, OFFSET_TRAP_RA(sp)

    // pointers, sp will be handled later

    // temporary
    ld t0, OFFSET_TRAP_T0(sp)
    ld t1, OFFSET_TRAP_T1(sp)
    ld t2, OFFSET_TRAP_T2(sp)


    // args
    ld a0, OFFSET_TRAP_A0(sp)
    ld a1, OFFSET_TRAP_A1(sp)
    ld a2, OFFSET_TRAP_A2(sp)
    ld a3, OFFSET_TRAP_A3(sp)
    ld a4, OFFSET_TRAP_A4(sp)
    ld a5, OFFSET_TRAP_A5(sp)
    ld a6, OFFSET_TRAP_A6(sp)
    ld a7, OFFSET_TRAP_A7(sp)

    // temporary register
    ld t3, OFFSET_TRAP_T3(sp)
    ld t4, OFFSET_TRAP_T4(sp)
    ld t5, OFFSET_TRAP_T5(sp)
    ld t6, OFFSET_TRAP_T6(sp)

    addi sp, sp, OFFSET_SIZE
.endm

#--------------------------------------------------------------------
# __dasics_uret: ret from user exception.
#--------------------------------------------------------------------
ENTRY(__dasics_uret)
    RESTORE_CONTEXT
    uret
END(__dasics_uret)

#--------------------------------------------------------------------
# dasics_ufault_entry: entry function before handling dasics faults.
#--------------------------------------------------------------------
ENTRY(dasics_ufault_entry) 
    SAVE_CONTEXT
    mv a0, sp
    la t0, dasics_ufault_handler
    la ra, __dasics_uret
    jr t0
    ret
END(dasics_ufault_entry)

#-----------------------------------------------------------------------
# __dasics_start_ufault_entry: deal fault when the dynamic.
#-----------------------------------------------------------------------
ENTRY(__dasics_start_ufault_entry)
//    SAVE_CONTEXT
//    mv a0, sp
//    la ra, __dasics_uret
//    la t0, _dasics_start_ufault_entry
//    jr t0
//    ret
END(__dasics_start_ufault_entry)

