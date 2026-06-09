#include <stdio.h>

#define TILE_SIZE 16

__global__ void matAdd(int dim, const float *A, const float *B, float* C) {

    /********************************************************************
     *
     * Compute C = A + B
     *   where A is a (dim x dim) matrix
     *   where B is a (dim x dim) matrix
     *   where C is a (dim x dim) matrix
     *
     ********************************************************************/

    /*************************************************************************/
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    int row = blockIdx.y * blockDim.y + threadIdx.y;

    if (row < dim && col < dim) {
        int idx = row * dim + col;
        C[idx] = A[idx] + B[idx];
    }
        
    /*************************************************************************/

}

void basicMatAdd(int dim, const float *A, const float *B, float *C)
{
    // Initialize thread block and kernel grid dimensions ---------------------

    const unsigned int BLOCK_SIZE = TILE_SIZE;
	
    /*************************************************************************/
    dim3 dim_block(BLOCK_SIZE, BLOCK_SIZE);
    dim3 dim_grid((dim + BLOCK_SIZE - 1) / BLOCK_SIZE,
                  (dim + BLOCK_SIZE - 1) / BLOCK_SIZE);

    /*************************************************************************/
	
	// Invoke CUDA kernel -----------------------------------------------------

    /*************************************************************************/
    matAdd<<<dim_grid, dim_block>>>(dim, A, B, C);
	
    /*************************************************************************/

}
