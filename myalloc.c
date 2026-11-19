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


/* ---- Task 3: my_malloc with first-fit ---- */

/* Return the first free block that is big enough, or NULL */
static block_t *find_fit(size_t need) {
    for (block_t *b = head; b != NULL; b = b->next)
        if (b->free && b->size >= need)
            return b;
    return NULL;
}
static void split(block_t *b, size_t need);
void *my_malloc(size_t size) {
    if (size == 0 || size > SIZE_MAX / 2)
        return NULL;
    size = align_up(size);

    block_t *b = find_fit(size);
    if (b == NULL) {                        /* nothing fits: ask the OS */
        if (grow(size) == NULL)
            return NULL;
        b = find_fit(size);
        if (b == NULL)
            return NULL;
    }
    split(b, size);                         /* Task 4: cut off the extra */
    b->free = 0;                            /* mark as in use */
    return payload(b);                      /* give back the user's part */
}


/* ---- TEMPORARY stubs: each gets replaced by a real version later ---- */

/* ---- Task 5: my_free and coalescing ---- */

static void say_err(const char *msg) {
    ssize_t r = write(2, msg, strlen(msg));  /* fd 2 = standard error */
    (void)r;
}

/* Merge every run of free blocks that touch each other in memory */
static void coalesce(void) {
    block_t *b = head;
    while (b != NULL) {
        block_t *n = b->next;
        if (b->free && n != NULL && n->free &&
            (char *)payload(b) + b->size == (char *)n) {
            b->size += HDR + n->size;       /* swallow the neighbour */
            b->next = n->next;              /* unlink it from the list */
            if (tail == n)
                tail = b;                   /* we swallowed the last block */
            /* stay on b: the block after it may be free too */
        } else {
            b = b->next;
        }
    }
}

void my_free(void *ptr) {
    if (ptr == NULL)
        return;
    block_t *b = header(ptr);
    if (b->magic != MAGIC) {
        say_err("my_free: invalid pointer\n");
        return;
    }
    if (b->free) {
        say_err("my_free: double free detected\n");
        return;
    }
    b->free = 1;
    coalesce();
}

/* ---- Task 6: my_calloc and my_realloc ---- */

void *my_calloc(size_t n, size_t size) {
    if (n != 0 && size > SIZE_MAX / n)
        return NULL;                        /* n * size would overflow */
    size_t total = n * size;
    void *p = my_malloc(total);
    if (p == NULL)
        return NULL;
    memset(p, 0, total);
    return p;
}

void *my_realloc(void *ptr, size_t size) {
    if (ptr == NULL)
        return my_malloc(size);             /* acts like malloc */
    if (size == 0) {
        my_free(ptr);                       /* acts like free */
        return NULL;
    }
    if (size > SIZE_MAX / 2)
        return NULL;

    block_t *b = header(ptr);
    if (b->magic != MAGIC || b->free) {
        say_err("my_realloc: invalid pointer\n");
        return NULL;
    }
    size_t need = align_up(size);

    /* Case: it already fits. Cut off any unused tail. */
    if (b->size >= need) {
        split(b, need);
        coalesce();
        return ptr;
    }

    /* Case: the next block is free, touches us in memory, and is big enough */
    block_t *n = b->next;
    if (n != NULL && n->free &&
        (char *)payload(b) + b->size == (char *)n &&
        b->size + HDR + n->size >= need) {
        b->size += HDR + n->size;           /* absorb the neighbour */
        b->next = n->next;
        if (tail == n)
            tail = b;
        split(b, need);
        coalesce();
        return ptr;
    }

    /* Otherwise: allocate new, copy, free old */
    void *np = my_malloc(size);
    if (np == NULL)
        return NULL;                        /* the old block stays valid */
    memcpy(np, ptr, b->size < size ? b->size : size);
    my_free(ptr);
    return np;
}
void heap_stats(heap_stats_t *out) {
    memset(out, 0, sizeof *out);
}

void heap_dump(void) {
}

void set_policy(fit_policy_t p) {
    (void)p;
}

int heap_check(void) {
    return 1;
}
/* Task 4: if block b is much bigger than needed, cut the extra into a new free block */
static void split(block_t *b, size_t need) {
    if (b->size < need + HDR + 16)
        return;                             /* leftover too small: keep whole block */

    block_t *rest = (block_t *)((char *)payload(b) + need);
    rest->size = b->size - need - HDR;
    rest->next = b->next;
    rest->free = 1;
    rest->magic = MAGIC;

    b->size = need;
    b->next = rest;
    if (tail == b)
        tail = rest;                        /* we split the last block */
}
