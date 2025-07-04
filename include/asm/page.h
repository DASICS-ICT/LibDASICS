#ifndef __ASM_PAGE_H
#define __ASM_PAGE_H

#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE - 1)
#define PAGE_SHIFT 12
#define PAGE_ALIGN_UP(x) (((x) + PAGE_MASK) & ~PAGE_MASK)
#define PAGE_ALIGN_DOWN(x) ((x) & ~PAGE_MASK)

#endif /* __ASM_PAGE_H */