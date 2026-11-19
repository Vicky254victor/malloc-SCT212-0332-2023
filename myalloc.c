#define _DEFAULT_SOURCE
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>      /* only snprintf is allowed here, never printf */
#include <unistd.h>
#include "myalloc.h"

#define MAGIC 0xC0FFEE

/* The label that sits in front of every block */
typedef struct block {
    size_t size;            /* payload size in bytes */
    struct block *next;     /* next block, in increasing address order */
    int free;               /* 1 = free, 0 = in use */
    int magic;              /* detects bad pointers */
} block_t;

/* Header size, rounded up to a multiple of 16 */
#define HDR ((sizeof(block_t) + 15) & ~(size_t)15)

/* Round n up to a multiple of 16 */
static size_t align_up(size_t n) {
    return (n + 15) & ~(size_t)15;
}

/* From a block's header address to the user's pointer */
static void *payload(block_t *b) {
    return (char *)b + HDR;
}

/* From the user's pointer back to the block's header */
static block_t *header(void *p) {
    return (block_t *)((char *)p - HDR);
}


/* ---- Task 2: growing the heap ---- */

static block_t *head = NULL;    /* first block in the list */
static block_t *tail = NULL;    /* last block in the list */
static size_t heap_total = 0;   /* total bytes obtained from the OS */

/* Ask the OS for more memory and add one free block to the end of the list */
static block_t *grow(size_t need) {
    size_t chunk = HDR + need;
    if (chunk < 65536)
        chunk = 65536;                      /* at least 64 KB per call */
    chunk = align_up(chunk);

    void *p = sbrk((intptr_t)(chunk + 16)); /* +16 spare bytes for alignment */
    if (p == (void *)-1)
        return NULL;                        /* the OS said no */

    /* round the start up to a multiple of 16 */
    uintptr_t start = ((uintptr_t)p + 15) & ~(uintptr_t)15;
    block_t *b = (block_t *)start;
    b->size = chunk - HDR;
    b->next = NULL;
    b->free = 1;
    b->magic = MAGIC;

    if (tail != NULL)
        tail->next = b;
    else
        head = b;                           /* the very first block */
    tail = b;

    heap_total += chunk + 16;
    return b;
}
