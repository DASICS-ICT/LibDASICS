#include <stddef.h>
#include <umaincall.h>
#include <utrap.h>
#include <dynamic.h>

/* The old dispatcher indexes this prefix directly, while the saved contexts
 * use the current NanhuV5 layout. Fail the build if either contract drifts. */
_Static_assert(offsetof(umain_elf_t, _local_got_table) == 0, "GOT table offset");
_Static_assert(offsetof(umain_elf_t, target_elf) == 16, "target ELF offset");
_Static_assert(offsetof(umain_elf_t, target_func_name) == 24, "name offset");
_Static_assert(offsetof(umain_elf_t, target_is_main) == 32, "trusted target offset");
_Static_assert(sizeof(struct umaincall) == OFFSET_UMAINCALL, "maincall size");
_Static_assert(offsetof(struct umaincall, t0) == OFFSET_UMAINCALL_T0, "maincall t0");
_Static_assert(offsetof(struct umaincall, sp) == OFFSET_UMAINCALL_SP, "maincall sp");
_Static_assert(offsetof(struct umaincall, return_pc) == OFFSET_UMAINCALL_RETURNPC, "maincall return PC");
_Static_assert(offsetof(struct umaincall, fpr) == OFFSET_UMAINCALL_FPR, "maincall FP");
_Static_assert(sizeof(struct ucontext_trap) == OFFSET_SIZE, "trap size");
_Static_assert(offsetof(struct ucontext_trap, return_pc) == OFFSET_TRAP_RETURNPC, "trap return PC");
_Static_assert(offsetof(struct ucontext_trap, freezone_return_pc) == OFFSET_TRAP_FZRETURNPC, "trap active return PC");
_Static_assert(offsetof(struct ucontext_trap, syscall_action) == OFFSET_TRAP_SYSCALL_ACTION, "trap proxy action");
