#include <malloc-free.h>
#include <stdlib.h>
#include <stdint.h>
#include <dasics_stdio.h>



static inline int sys_write(const char * buff)
{

    int len = 0;
    for (; buff[len] != '\0'; len++);
    
    register long a7 asm("a7") = 64;
    register long a0 asm("a0") = 1;
    register long a1 asm("a1") = (unsigned long)buff;
    register long a2 asm("a2") = len;
    asm volatile("ecall"                      \
                 : "+r"(a0)                   \
                 : "r"(a1), "r"(a2)           \
                    ,"r"(a7)                  \
                 : "memory");

    return a0;

}


static unsigned int mini_itoa(
    long value, unsigned int radix, unsigned int uppercase,
    unsigned int unsig, char *buffer, unsigned int zero_pad)
{
    char *pbuffer = buffer;
    int negative  = 0;
    unsigned int i, len;

    /* No support for unusual radixes. */
    if (radix > 16) return 0;

    if (value < 0 && !unsig) {
        negative = 1;
        value    = -value;
    }

    /* This builds the string back to front ... */
    do {
        int digit = value % radix;
        *(pbuffer++) =
            (digit < 10 ? '0' + digit :
                          (uppercase ? 'A' : 'a') + digit - 10);
        value /= radix;
    } while (value > 0);

    for (i = (pbuffer - buffer); i < zero_pad; i++)
        *(pbuffer++) = '0';

    if (negative) *(pbuffer++) = '-';

    *(pbuffer) = '\0';

    /* ... now we reverse it (could do it recursively but will
     * conserve the stack space) */
    len = (pbuffer - buffer);
    for (i = 0; i < len / 2; i++) {
        char j              = buffer[i];
        buffer[i]           = buffer[len - i - 1];
        buffer[len - i - 1] = j;
    }

    return len;
}

struct mini_buff
{
    char *buffer, *pbuffer;
    unsigned int buffer_len;
};

static int _putc(int ch, struct mini_buff *b)
{
    if ((unsigned int)((b->pbuffer - b->buffer) + 1) >=
        b->buffer_len)
        return 0;
    *(b->pbuffer++) = ch;
    *(b->pbuffer)   = '\0';
    return 1;
}

static int _puts(char *s, unsigned int len, struct mini_buff *b)
{
    unsigned int i;

    if (b->buffer_len - (b->pbuffer - b->buffer) - 1 < len)
        len = b->buffer_len - (b->pbuffer - b->buffer) - 1;

    /* Copy to buffer */
    for (i = 0; i < len; i++) *(b->pbuffer++) = s[i];
    *(b->pbuffer) = '\0';

    return len;
}

static int mini_vsnprintf(
    char *buffer, unsigned int buffer_len, const char *fmt,
    va_list va)
{
    struct mini_buff b;
    char bf[24];
    char ch;

    b.buffer     = buffer;
    b.pbuffer    = buffer;
    b.buffer_len = buffer_len;

    while ((ch = *(fmt++))) {
        if ((unsigned int)((b.pbuffer - b.buffer) + 1) >=
            b.buffer_len)
            break;
        if (ch != '%')
            _putc(ch, &b);
        else {
            char zero_pad = 0;
            int longflag = 0;
            char *ptr;
            unsigned int len;

            ch = *(fmt++);

            /* Zero padding requested */
            if (ch == '0') {
                while ((ch = *(fmt++))) {
                    if (ch == '\0') goto end;
                    if (ch >= '0' && ch <= '9') {
                        zero_pad = zero_pad * 10 + ch - '0';
                    } else {
                        break;
                    }
                }
            }
            if (ch == 'l') {
                longflag = 1;
                ch = *(fmt++);
            }

            switch (ch) {
                case 0:
                    goto end;

                case 'l':
                    longflag = 1;
                    break;

                case 'u':
                case 'd':
                    len = mini_itoa(
                        longflag == 0 ? (unsigned long)va_arg(
                                            va, unsigned int) :
                                        va_arg(va, unsigned long),
                        10, 0, (ch == 'u'), bf, zero_pad);
                    _puts(bf, len, &b);
                    longflag = 0;
                    break;
                case 'x':
                case 'X':
                    len = mini_itoa(
                        longflag == 0 ? (unsigned long)va_arg(
                                            va, unsigned int) :
                                        va_arg(va, unsigned long),
                        16, (ch == 'X'), 1, bf, zero_pad);
                    _puts(bf, len, &b);
                    longflag = 0;
                    break;

                case 'c':
                    _putc((char)(va_arg(va, int)), &b);
                    break;

                case 's':
                    ptr = va_arg(va, char *);
                    _puts(ptr, dasics_strlen(ptr), &b);
                    break;

                default:
                    _putc(ch, &b);
                    break;
            }
        }
    }
end:
    return b.pbuffer - b.buffer;
}

int lib_vprintf(const char *fmt, va_list _va)
{
    va_list va;
    va_copy(va, _va);

    int ret;
    char buff[256];

    ret = mini_vsnprintf(buff, 256, fmt, va);

    buff[ret] = '\0';

    sys_write(buff);

    return ret;
}

int lib_printf(const char *fmt, ...)
{
    int ret = 0;
    va_list va;

    va_start(va, fmt);
    ret = lib_vprintf(fmt, va);
    va_end(va);

    return ret;
}


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
        lib_printf("[FREE]: addr: %lx\n", (uint64_t)tmp);

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

        lib_printf("[FREE]: addr: %lx\n", (uint64_t)point);
        lib_printf("[FREE]: data: %s\n", point->data);
        point->prev->next = point->next;
        point->next->prev = point->prev;
        free((void *)point);
        lib_printf("[FREE]: Try to access free addr: %lx\n", (uint64_t)point);
        lib_printf("[FREE]: Get free data: ");
        for (int i = 0; i < MSG_LEN; i++)
        {
            /* code */
            lib_printf("%c", point->data[i]);     

        }
        lib_printf("\n");
    }  

    // REALLOC
    if (flag == REALLOC)
    {

        if (head == NULL) return;

        malloc_free_t * point = head->prev;

        lib_printf("[FREE]: addr: %lx\n", (uint64_t)point);
        lib_printf("[FREE]: data: %s\n", point->data);

        malloc_free_t * record = point;


        point->prev->next = point->next;
        point->next->prev = point->prev;

        point = (malloc_free_t *)realloc(point, MALLOC_SIZE + 8);

        // Add chain
        point->next = head;
        point->prev = head->prev;
        head->prev->next = point;
        head->prev = point;


        lib_printf("[FREE]: Try to access free addr: %lx, point: %lx\n", (uint64_t)record, (uint64_t)point);
        lib_printf("[FREE]: Get free data: ");
        for (int i = 0; i < MSG_LEN; i++)
        {
            /* code */
            lib_printf("%c", record->data[i]);     

        }
        lib_printf("\n");
    }
}