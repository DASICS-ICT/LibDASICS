#ifndef _INCLUDE_UWRAPPER_H
#define _INCLUDE_UWRAPPER_H
#include <stdint.h>

extern void dasics_ulib_libcall(void *arg0, void *arg1, void *arg2, void *arg3, void *func_name);
#define dasics_ulib_libcall_no_args(func_name) (dasics_ulib_libcall(0, 0, 0, 0, func_name))

extern void dasics_ulib_copy_mem_bound(int bound_src, int bound_dst);
extern void dasics_ulib_copy_jmp_bound(int bound_src, int bound_dst);
extern uint64_t dasics_ulib_query_mem_bound(void);
extern uint64_t dasics_ulib_query_jmp_bound(void);

#define dasics_ulib_copy_bound(type,bound_src,bound_dst) ((type)?\
                                                    dasics_ulib_copy_jmp_bound(bound_src, bound_dst):\
                                                    dasics_ulib_copy_mem_bound(bound_src, bound_dst))
#define dasics_ulib_query_bound(type) ((type)?\
                                  dasics_ulib_query_jmp_bound():\
                                  dasics_ulib_query_mem_bound())


#endif