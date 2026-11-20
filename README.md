# malloc: my own memory allocator in C

Name: Victor Munene Njeru
Student ID: SCT212-0332/2023

## Build and run

    make          # builds and runs the provided tests
    make bench    # builds the benchmark
    ./bench first
    ./bench best
    ./bench libc

## Tasks

| Task | Description | Status |
|---|---|---|
| 1 | Block header and helpers | complete |
| 2 | Growing the heap with sbrk | complete |
| 3 | my_malloc with first-fit | complete |
| 4 | Splitting blocks | complete |
| 5 | my_free and coalescing | complete |
| 6 | my_calloc and my_realloc | complete |
| 7 | Best-fit, stats, dump and check | complete |
| 8 | Benchmark and report (see REPORT.md) | complete |

## Test results (last lines of make)

~~~
  PASS  no data corruption during 20000 operations
  PASS  everything freed
  PASS  fully coalesced (few free blocks remain)
  PASS  heap consistent

Result: 29 passed, 0 failed
~~~

## Design explanation

Each block starts with a header of four fields: size (the payload size in bytes), next (a pointer to the next block), free (1 or 0) and magic (a constant used to spot bad pointers). The header is 24 bytes, which I round up to 32 so that HDR is a multiple of 16. The pointer given to the user is the header address plus HDR, so it is always 16-byte aligned.

All blocks, free and used, are kept in one singly linked list in increasing address order, with global head and tail pointers. When no block fits, grow() asks the OS for at least 64 KB with sbrk(), aligns the start, and adds one big free block to the end of the list. my_malloc rounds the size up to a multiple of 16, finds a block with find_fit (first-fit or best-fit, chosen with set_policy), splits it if the leftover can hold a header plus 16 bytes, and marks it used.

my_free checks the magic number and the free flag to catch invalid pointers and double frees, marks the block free, then calls coalesce(). coalesce walks the whole list and merges two neighbours only if the first block's payload ends exactly where the next header begins. This address check matters because libc's malloc can move the break and leave gaps between my chunks. When two blocks merge, the first one grows by HDR plus the neighbour's size, the neighbour is unlinked, and tail is updated if the neighbour was the last block.
