#include <stdio.h>

__global__ void histo_kernel(unsigned int* input, unsigned int* bins, unsigned int num_elements, unsigned int num_bins)
{
    extern __shared__ unsigned int s_bins[];

    for (unsigned int b = threadIdx.x; b < num_bins; b += blockDim.x) {
        s_bins[b] = 0;
    }
    __syncthreads();

    unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int stride = blockDim.x * gridDim.x;
    for (unsigned int i = tid; i < num_elements; i += stride) {
        atomicAdd(&s_bins[input[i]], 1);
    }
    __syncthreads();

    for (unsigned int b = threadIdx.x; b < num_bins; b += blockDim.x) {
        atomicAdd(&bins[b], s_bins[b]);
    }
}

void histogram(unsigned int* input, unsigned int* bins, unsigned int num_elements, unsigned int num_bins) {
    const unsigned int BLOCK_SIZE = 512;
    unsigned int gridSize = (num_elements + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (gridSize > 1024) gridSize = 1024;

    size_t shmem_bytes = num_bins * sizeof(unsigned int);
    histo_kernel<<<gridSize, BLOCK_SIZE, shmem_bytes>>>(input, bins, num_elements, num_bins);
}
