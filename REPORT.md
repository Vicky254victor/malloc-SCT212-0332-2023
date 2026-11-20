# Allocator Report

## Workload

bench.c runs 50,000 operations on 1,000 slots. Each step picks a random slot: if it holds a block it is freed, otherwise a block of random size (1 to 4096 bytes) is allocated. The seed is fixed (srand(12345)), so first-fit, best-fit and glibc malloc all see exactly the same sequence. Each policy is run as a separate program run so one heap does not affect the other.

## Results

| Policy | heap_bytes | free_bytes | free_blocks | largest_free |
|---|---|---|---|---|
| First-fit | 1376592 | 321344 | 235 | 65504 |
| Best-fit | 1311040 | 256816 | 175 | 65504 |

## External fragmentation

Formula: 1 - largest_free / free_bytes

| Policy | Fragmentation |
|---|---|
| First-fit | 0.7962 |
| Best-fit | 0.7449 |

## Timing (50,000 operations)

| Allocator | Time (s) |
|---|---|
| First-fit | 0.1973 |
| Best-fit | 0.2508 |
| glibc malloc/free | 0.0188 |

## Discussion


vv
From my results I would choose best-fit. It used about 65 KB less heap than first-fit (1,311,040 bytes against 1,376,592) and left fewer free blocks (175 against 235). Its external fragmentation was also a bit lower, 0.7449 compared to 0.7962. The price is speed: best-fit took 0.2508 s and first-fit took 0.1973 s, because best-fit has to walk the whole list on every allocation instead of stopping at the first block that fits. For a small training lab, saving memory matters more to me than a fraction of a second, so I would go with best-fit.

My allocator is also much slower than glibc, which did the same work in 0.0188 s. One reason is that my my_free scans the entire list to merge neighbours, so every free is O(n). glibc keeps free blocks in bins by size and stores size information at the block boundaries, so it finds what it needs without walking everything.

cat >> REPORT.md << 'EOF'
From my results I would choose best-fit. It used about 65 KB less heap than first-fit (1,311,040 bytes against 1,376,592) and left fewer free blocks (175 against 235). Its external fragmentation was also a bit lower, 0.7449 compared to 0.7962. The price is speed: best-fit took 0.2508 s and first-fit took 0.1973 s, because best-fit has to walk the whole list on every allocation instead of stopping at the first block that fits. For a small training lab, saving memory matters more to me than a fraction of a second, so I would go with best-fit.

My allocator is also much slower than glibc, which did the same work in 0.0188 s. One reason is that my my_free scans the entire list to merge neighbours, so every free is O(n). glibc keeps free blocks in bins by size and stores size information at the block boundaries, so it finds what it needs without walking everything.
