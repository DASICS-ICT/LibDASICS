#include <malloc-free.h>
#include <stdlib.h>
#include <stdint.h>
#include <dasics_stdio.h>
#include <dasics_string.h>

malloc_free_t * head = NULL;
int num = 0;

/*
 * This function is used to do memcpy in the _dasics_entry_stage2 to avoid
 * to use glibic memecpy
 */
static inline int __dasics_linker_memcpy(char *dest, const char *src, unsigned int len)
{
    for (int i = 0; i < len; i++)
    {
        dest[i] = src[i];
    }
    return len;
}

static inline int __dasics_linker_strcpy(char *dest, const char *src)
{
    char *tmp = dest;

    while (*src) {
        *dest++ = *src++;
    }

    *dest = '\0';

    return tmp;
}

static inline int __dasics_linker_strcmp(const char *str1, const char *str2)
{
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            return (*str1) - (*str2);
        }
        ++str1;
        ++str2;
    }
    return (*str1) - (*str2);
}


int call_and_record(int flag, char * message)
{            
    // Malloc 
    if (flag == MALLOC)
    {
        malloc_free_t * tmp = (malloc_free_t *)malloc(MALLOC_SIZE);
        dasics_printf("[FREE]: addr: %lx\n", (uint64_t)tmp);

        __dasics_linker_strcpy(tmp->data, message);

        num++;
        if (head == NULL)
        {
            head = tmp;
            head->next = head->prev = head;
        } else 
        {
            tmp->next = head;
            tmp->prev = head->prev;
            head->prev->next = tmp;
            head->prev = tmp;
        }
    }

    // Free
    if (flag == FREE)
    {
        if (head == NULL) return;
        malloc_free_t * point = head->prev;

        dasics_printf("[FREE]: addr: %lx\n", (uint64_t)point);
        dasics_printf("[FREE]: data: %s\n", point->data);
        point->prev->next = point->next;
        point->next->prev = point->prev;
        free((void *)point);
        dasics_printf("[FREE]: Try to access free addr: %lx\n", (uint64_t)point);
        dasics_printf("[FREE]: Get free data: ");
        for (int i = 0; i < MSG_LEN; i++)
        {
            /* code */
            dasics_printf("%c", point->data[i]);     

        }
        dasics_printf("\n");
    }  

    // REALLOC
    if (flag == REALLOC)
    {

        if (head == NULL) return;

        malloc_free_t * point = head->prev;

        dasics_printf("[FREE]: addr: %lx\n", (uint64_t)point);
        dasics_printf("[FREE]: data: %s\n", point->data);

        malloc_free_t * record = point;


        point->prev->next = point->next;
        point->next->prev = point->prev;

        point = (malloc_free_t *)realloc(point, MALLOC_SIZE + 8);

        // Add chain
        point->next = head;
        point->prev = head->prev;
        head->prev->next = point;
        head->prev = point;


        dasics_printf("[FREE]: Try to access free addr: %lx, point: %lx\n", (uint64_t)record, (uint64_t)point);
        dasics_printf("[FREE]: Get free data: ");
        for (int i = 0; i < MSG_LEN; i++)
        {
            /* code */
            dasics_printf("%c", record->data[i]);     

        }
        dasics_printf("\n");
    }
}