#include <stdio.h>
#include <udasics.h>
#include <uattr.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dasics_stdio.h>
#include <link.h>
#include <elf.h>

#define BOUNDS 16


void ATTR_UFREEZONE_TEXT Malicious(void * unsed);
void ATTR_UFREEZONE_TEXT homebrew_memcpy(void * dst, const void * src, size_t length);


#define OLD_BP_PTR   __builtin_frame_address(0)
#define RET_ADDR_PTR ((void **) OLD_BP_PTR - 1)


char * bounds[BOUNDS] = {NULL};



static char data_secret[32] = "success. Secret data leaked.\n";


int main(int argc, char * argv[])
{
    // register_udasics(0);

    // dasics_libcfg_free_all();
    
    // for (int i = 0; i < BOUNDS; i++)
    // {
    //     char * buff = (char *)malloc(256);
    //     assert(buff != NULL);

    //     sprintf(buff, "This is %d test buffer\n", i);

    //     bounds[i] = buff;

    //     // dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_V, (uint64_t)buff, (uint64_t)buff + 256);

    // }
    
    // // dasics_libcfg_alloc(DASICS_LIBCFG_R |DASICS_LIBCFG_V, (uint64_t)bounds, (uint64_t)bounds + sizeof(bounds) + 1);

    // register uint64_t sp asm("sp");

    // // dasics_libcfg_alloc(DASICS_LIBCFG_R | DASICS_LIBCFG_W |DASICS_LIBCFG_V, (uint64_t)sp - 0x3000, sp);

    // // lib_call(Malicious, NULL);

    // Malicious(NULL);

    // dasics_printf("[END]\n");
     


    // unregister_udasics();

    // Elf64_Dyn * dyn =  NULL;
    // struct r_debug * debug_extended =  NULL;
    // for(dyn = _DYNAMIC; dyn->d_tag != DT_NULL; ++dyn)
    // {
    //     if(dyn->d_tag == DT_DEBUG)
    //     {
    //         debug_extended = (struct r_debug *)dyn->d_un.d_ptr;
    //     }          
    // }
    
    // struct link_map * link = debug_extended->r_map;

    // for (struct link_map *cur = link; cur != NULL ; cur = cur->l_next)
    // {
    //     printf("[LIB]: %s, l_addr: 0x%lx\n", cur->l_name, cur->l_addr);
    // }
    


    return 0;
}

void
rop_target()
{
    printf("success.\nROP function reached.\n");
    exit(0);
}

void ATTR_UFREEZONE_TEXT 
Malicious(void * unsed)
{

    char local[256];
    for (int i = 0; i < BOUNDS; i++)
    {
        homebrew_memcpy(local, bounds[i], 256);

        printf(local);
        // dasics_umaincall(Umaincall_PRINT, local);
    }


    for (int i = BOUNDS - 1; i >= 0; i--)
    {
        homebrew_memcpy(local, bounds[i], 256);

        // dasics_umaincall(Umaincall_PRINT, local);
        printf(local);
    }    
    
    return;

}

#pragma GCC push_options
#pragma GCC optimize("O0")

void ATTR_UFREEZONE_TEXT
homebrew_memcpy(void * dst, const void * src, size_t length)
{
    char * d, * s;

    d = (char *) dst;
    s = (char *) src;

    while (length--) {
        *d++ = *s++;
    }
}
#pragma GCC pop_options
