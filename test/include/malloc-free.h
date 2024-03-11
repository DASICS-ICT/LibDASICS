#ifndef MALLOC_FREE_H
#define MALLOC_FREE_H

/* Used to test malloc-free  */
#define MALLOC  0
#define FREE    1
#define REALLOC 2
#define MSG_LEN 64

typedef struct malloc_free
{
    char data[MSG_LEN];
    struct malloc_free *prev;
    struct malloc_free *next;
} malloc_free_t;

#define MALLOC_SIZE sizeof(struct malloc_free)

extern struct malloc_free * head;


int call_and_record(int flag, char * message);







#endif