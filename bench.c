#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "myalloc.h"

#define OPS    50000
#define SLOTS  1000
#define MAXSZ  4096
#define SEED   12345

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s first|best|libc\n", argv[0]);
        return 1;
    }
    int use_libc = 0;
    if (strcmp(argv[1], "first") == 0)
        set_policy(FIT_FIRST);
    else if (strcmp(argv[1], "best") == 0)
        set_policy(FIT_BEST);
    else if (strcmp(argv[1], "libc") == 0)
        use_libc = 1;
    else {
        fprintf(stderr, "usage: %s first|best|libc\n", argv[0]);
        return 1;
    }

    static void *slot[SLOTS];
    srand(SEED);
    double t0 = now();
    for (int i = 0; i < OPS; i++) {
        int k = rand() % SLOTS;
        if (slot[k] != NULL) {
            if (use_libc) free(slot[k]); else my_free(slot[k]);
            slot[k] = NULL;
        } else {
            size_t sz = 1 + (size_t)(rand() % MAXSZ);
            slot[k] = use_libc ? malloc(sz) : my_malloc(sz);
            if (slot[k] != NULL)
                *(char *)slot[k] = 1;
        }
    }
    double t1 = now();

    if (use_libc) {
        printf("%-5s time=%.4f s\n", argv[1], t1 - t0);
    } else {
        heap_stats_t s;
        heap_stats(&s);
        double frag = (s.free_bytes > 0)
            ? 1.0 - (double)s.largest_free / (double)s.free_bytes : 0.0;
        printf("%-5s heap_bytes=%zu free_bytes=%zu free_blocks=%zu "
               "largest_free=%zu ext_frag=%.4f time=%.4f s heap_check=%d\n",
               argv[1], s.heap_bytes, s.free_bytes, s.free_blocks,
               s.largest_free, frag, t1 - t0, heap_check());
    }

    for (int i = 0; i < SLOTS; i++)
        if (slot[i] != NULL) {
            if (use_libc) free(slot[i]); else my_free(slot[i]);
        }
    return 0;
}
