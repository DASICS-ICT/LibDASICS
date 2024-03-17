#include <dmalloc.h>
#include <dasics_stdio.h>
#include <dasics_string.h>

uint64_t mmap_base = 0;
uint64_t mmap_top = 0;

static void * mmap_chunk()
{
    int64_t addr = (int64_t)_dasics_mmap(0, \
                            CHUNK_SIZE, \
                            PROT_READ | PROT_WRITE, \
                            MAP_PRIVATE | MAP_ANONYMOUS, 
                            -1,
                            0);
    if (addr <= 0) 
    {
        dasics_printf("[Error]: DASICS mmap error");
        while(1);
    }
    mmap_base = (uint64_t)addr;
    mmap_top = mmap_base + CHUNK_SIZE;    

    return (void *)mmap_base;
}


void * dasics_malloc(uint64_t size)
{
    uint64_t base = 0;

    if (!mmap_base) 
    {
        mmap_chunk();
    }

    mmap_base = ROUND(mmap_base, 0x16);

    if (mmap_top - mmap_base < size)
        mmap_chunk();

    base = mmap_base;
    mmap_base += size;

    return (void *)base;
}
