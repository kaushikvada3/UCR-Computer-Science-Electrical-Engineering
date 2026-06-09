# Matrix addition writeup

## 1. How many total thread blocks do we use?

Block size is 16×16, so 256 threads per block.

Grid size in each dimension is `(dim + 15) / 16`, i.e. round dim up to a multiple of 16.

Total blocks = that number squared.

For the default `./mat-add` with dim = 1000: (1000 + 15) / 16 = 63, so 63 × 63 = **3969 blocks**.

For dim = 1024: 64 × 64 = **4096 blocks**.

## 2. Are all thread blocks full? Does every thread have work?

Every block still has 256 threads — CUDA always launches a full block.

But not every thread always does useful work. The kernel only writes if `row < dim` and `col < dim`. If dim is not divisible by 16, the grid is bigger than the matrix, so threads on the right/bottom edge hit the `if` and do nothing.

So for 1000×1000, some threads in the edge blocks are idle. For 1024×1024, every thread maps to a real element.

Rough numbers for 1000×1000: 63×63×256 = 1,016,064 threads launched, but only 1,000,000 outputs, so about 16k threads do no write.

## 3. How could we speed it up? What did I try / see?

Matrix add is mostly memory bound (read A, read B, write C), so you care about memory and less wasted threads, not clever math.

Ideas that could help:

- Use dim divisible by 16 so you don’t launch extra idle threads on the edge.
- Try different block sizes (still 2D) and see what’s faster on the GPU.
- Use float4 loads/stores if rows are aligned so you move more data per instruction.
- Time the kernel with CUDA events instead of relying on the printed “Launching kernel” line — that line includes sync and first-run stuff.

I didn’t change the kernel code yet; I only ran the stock program on **bender**.

**One run: `./mat-add` (1000×1000)**

- Setup: 0.030646 s  
- cudaMalloc: 0.320240 s (first run pays a lot for CUDA setup)  
- H2D copy: 0.001909 s  
- Launching kernel: 0.039790 s  
- D2H copy: 0.007172 s  
- `TEST PASSED 491` — that number is just how `verify()` in support.cu prints; it adds up part of C per row. It matched the expected output; the important part is it said PASSED not FAILED.

**What I noticed:** The big hit on the first run was allocation, not the kernel. The “Launching kernel” time probably includes overhead (sync, maybe first launch). I’d run it twice and compare, or use events, if I wanted a fair kernel time.
