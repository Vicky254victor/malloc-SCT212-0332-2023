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

## Questions

1. Why must the header size be a multiple of 16 as well as the payload sizes?

The user pointer is the header address plus the header size. Every block starts on a 16-byte boundary, so the payload is only aligned if the header size is also a multiple of 16. My header is 24 bytes, so without rounding it up to 32 the payload would sit 8 bytes off.

2. Why can printf() inside my_malloc() cause infinite recursion?

printf can call malloc internally, for example to create its output buffer. If this allocator were used as the real malloc, printf would call my_malloc again, which would call printf again, without end. It would also re-enter the allocator while the list is only half updated. That is why I use snprintf into a local array and write().

3. Your my_free() is O(n). Which extra field would let you find a block's previous neighbor in O(1), and what is the name of the technique?

A footer at the end of each block that stores its size, or a prev pointer in the header. The technique is called boundary tags. With it, free can look at the block before and the block after directly and merge them without scanning the list.

4. Explain the difference between internal and external fragmentation, and name one place in your allocator where each one comes from.

Internal fragmentation is wasted space inside an allocated block. In my allocator it comes from rounding sizes up to a multiple of 16, and from giving away a whole block when the leftover is too small to split. External fragmentation is free space broken into many small pieces that cannot satisfy a large request. It comes from freeing blocks that sit between used ones, and my benchmark showed it clearly, with first-fit at about 0.80.

5. Why does sbrk() memory never get returned to the OS in your allocator? Describe how you could give back memory at the end of the heap.

My my_free only marks a block as free and merges it, and I never call sbrk with a negative number. To give memory back, I could check whether the tail block is free and ends exactly at the current break. If it is large enough, I would shrink it, call sbrk with the negative size, and update tail and the heap total.

6. Two blocks are consecutive in your list but not adjacent in memory. How did this happen, and what does your coalescing code do about it?

It can happen when libc's malloc, which printf uses, moves the program break between two of my grow() calls, or because of the alignment padding. My coalesce code only merges two blocks if the first payload plus its size equals the address of the next header, so blocks with a gap between them are left alone.

## Known limitations

- my_free scans the whole list every time, so it is O(n).
- Best-fit scans the whole list on every allocation, which makes it slower.
- Memory is never returned to the OS.
- It is not thread-safe.

## Sources and AI-tool declaration

- The assignment PDF and the provided starter pack (myalloc.h, test_myalloc.c, Makefile).
- Claude (Anthropic's AI assistant): I used it for step-by-step guidance, help with the code, fixing compiler errors, drafting the README text and the REPORT.md discussion, and fixing my git repository. I ran and tested all the code myself.
