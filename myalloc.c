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
