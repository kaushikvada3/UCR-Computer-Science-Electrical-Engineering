#include <stdio.h>

#define TILE_SIZE 16

__global__ void mysgemm(int m, int n, int k, const float *A, const float *B, float* C) {

    /********************************************************************
     *
     * Compute C = A x B
     *   where A is a (m x k) matrix
     *   where B is a (k x n) matrix
     *   where C is a (m x n) matrix
     *
     * Use shared memory for tiling
     *
     ********************************************************************/

    __shared__ float tileA[TILE_SIZE][TILE_SIZE];
    __shared__ float tileB[TILE_SIZE][TILE_SIZE];

    int tx = threadIdx.x, ty = threadIdx.y;
    int row = blockIdx.y * TILE_SIZE + ty;
    int col = blockIdx.x * TILE_SIZE + tx;

    float sum = 0.0f;
    int numTiles = (k + TILE_SIZE - 1) / TILE_SIZE;

    for (int t = 0; t < numTiles; t++) {
        int aCol = t * TILE_SIZE + tx;
        int bRow = t * TILE_SIZE + ty;

        tileA[ty][tx] = (row < m && aCol < k) ? A[row * k + aCol] : 0.0f;
        tileB[ty][tx] = (bRow < k && col < n) ? B[bRow * n + col] : 0.0f;

        __syncthreads();

        for (int i = 0; i < TILE_SIZE; i++)
            sum += tileA[ty][i] * tileB[i][tx];

        __syncthreads();
    }

    if (row < m && col < n)
        C[row * n + col] = sum;
}

void basicSgemm(int m, int n, int k, const float *A, const float *B, float *C)
{
    // Initialize thread block and kernel grid dimensions ---------------------

    const unsigned int BLOCK_SIZE = TILE_SIZE;

    dim3 dim_block(BLOCK_SIZE, BLOCK_SIZE);
    dim3 dim_grid((n + BLOCK_SIZE - 1) / BLOCK_SIZE, (m + BLOCK_SIZE - 1) / BLOCK_SIZE);

    // Invoke CUDA kernel -----------------------------------------------------

    mysgemm<<<dim_grid, dim_block>>>(m, n, k, A, B, C);
}


