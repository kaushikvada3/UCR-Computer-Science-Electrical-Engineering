import matplotlib.pyplot as plt
import numpy as np

# Part 2 Data
sizes = [1024, 2048, 4096, 8192]
row_times = [0.002, 0.007, 0.028, 0.111]
col_times = [0.006, 0.065, 0.571, 1.546]

plt.figure(figsize=(10, 6))
plt.plot(sizes, row_times, marker='o', label='Row-Major')
plt.plot(sizes, col_times, marker='s', label='Column-Major')
plt.xscale('log', base=2)
# User requested "Plot array size (x-axis) vs execution time (y-axis)".
# Linear Y-axis shows the magnitude difference better for this specific data range (0.1 vs 1.5).
plt.xlabel('Array Size (N)')
plt.ylabel('Execution Time (seconds)')
plt.title('Row vs Column Major Access Performance')
plt.legend()
plt.grid(True)
plt.savefig('rowcol_plot.png')
plt.close()

# Part 3 Data
strides = [4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192]
times_stride = [0.023, 0.028, 0.042, 0.069, 0.174, 0.227, 0.381, 0.228, 0.295, 0.247, 0.280, 0.435]

plt.figure(figsize=(10, 6))
plt.plot(strides, times_stride, marker='o')
plt.xscale('log', base=2)
# User requested "Plot stride (x-axis, log scale) vs execution time (y-axis)".
plt.xlabel('Stride (bytes)')
plt.ylabel('Execution Time (seconds)')
plt.title('Stride Access Performance')
plt.grid(True)
plt.savefig('stride_plot.png')
plt.close()

# Part 4 Data
# Naive Data
naive_sizes = [256, 384, 512]
naive_times = [0.108, 0.312, 0.595]

# Blocked Data
block_sizes = [16, 32, 64, 96, 128]
# N=256
blocked_256 = [0.089, 0.093, 0.089, 0.084, 0.089]
# N=384
blocked_384 = [0.208, 0.224, 0.214, 0.220, 0.220]
# N=512
blocked_512 = [0.499, 0.537, 0.527, 0.533, 0.530]

# Plot 1: Naive Runtime vs Matrix Size
plt.figure(figsize=(10, 6))
plt.plot(naive_sizes, naive_times, marker='o', color='red', label='Naive')
plt.xlabel('Matrix Size (N)')
plt.ylabel('Execution Time (seconds)')
plt.title('Naive Matrix Multiplication Runtime')
plt.grid(True)
plt.legend()
plt.savefig('naive_plot.png')
plt.close()

# Plot 2: Blocked Runtime vs Block Size
plt.figure(figsize=(10, 6))
plt.plot(block_sizes, blocked_256, marker='o', label='N=256')
plt.plot(block_sizes, blocked_384, marker='s', label='N=384')
plt.plot(block_sizes, blocked_512, marker='^', label='N=512')
plt.xlabel('Block Size')
plt.ylabel('Execution Time (seconds)')
plt.title('Blocked Matrix Multiplication Runtime')
plt.grid(True)
plt.legend()
plt.savefig('blocked_plot.png')
plt.close()
