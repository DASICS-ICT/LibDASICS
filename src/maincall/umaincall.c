#include <umaincall.h>
#include <udasics.h>
#include <dynamic.h>
#include <dasics_start.h>
#include <dasics_stdio.h>

int _open_maincall()
{
    umaincall_helper = (uint64_t)dasics_umaincall_helper;
    csr_write(dmaincall, (uint64_t)dasics_umaincall);
    return 0;
}

