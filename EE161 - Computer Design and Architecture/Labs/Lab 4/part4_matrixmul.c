#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Naive Matrix Multiplication
void matmul_naive(double **A, double **B, double **C, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double sum = 0.0;
            for (int k = 0; k < n; k++)
                sum += A[i][k] * B[k][j];
            C[i][j] = sum;
        }
    }
}

// Blocked Matrix Multiplication
void matmul_blocked(double **A, double **B, double **C, int n, int block) {
    // Initialize C to 0 first because we accumulate into it? 
    // The provided snippet in previous turn showed:
    // double sum = C[i][j]; ... C[i][j] = sum;
    // This implies C must be initialized or we must handle the first block differently.
    // Standard blocked matmul usually accumulates. 
    // Let's ensure C is zeroed before calling this or inside.
    // Actually, the standard blocked algorithm iterates loops:
    // for ii, jj, kk...
    //   for i, j, k...
    //     C[i][j] += A[i][k] * B[k][j]
    // So yes, C needs to be 0 initially.
    
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            C[i][j] = 0.0;

    for (int ii = 0; ii < n; ii += block) {
        for (int jj = 0; jj < n; jj += block) {
            for (int kk = 0; kk < n; kk += block) {
                for (int i = ii; i < ii + block && i < n; i++) {
                    for (int j = jj; j < jj + block && j < n; j++) {
                        double sum = C[i][j];
                        for (int k = kk; k < kk + block && k < n; k++) {
                            sum += A[i][k] * B[k][j];
                        }
                        C[i][j] = sum;
                    }
                }
            }
        }
    }
}

double** allocate_matrix(int n) {
    double **mat = (double **)malloc(n * sizeof(double *));
    for (int i = 0; i < n; i++)
        mat[i] = (double *)malloc(n * sizeof(double));
    return mat;
}

void free_matrix(double **mat, int n) {
    for (int i = 0; i < n; i++)
        free(mat[i]);
    free(mat);
}

void init_matrix(double **mat, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            mat[i][j] = 1.0; // Simple initialization
}

int main() {
    int sizes[] = {256, 384, 512};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    int blocks[] = {16, 32, 64, 96, 128};
    int num_blocks = sizeof(blocks) / sizeof(blocks[0]);

    printf("N\tType\tBlockSize\tTime(s)\n");

    for (int s = 0; s < num_sizes; s++) {
        int N = sizes[s];
        
        double **A = allocate_matrix(N);
        double **B = allocate_matrix(N);
        double **C = allocate_matrix(N);
        
        init_matrix(A, N);
        init_matrix(B, N);

        // Naive Run
        clock_t start = clock();
        matmul_naive(A, B, C, N);
        clock_t end = clock();
        double naive_time = ((double)(end - start)) / CLOCKS_PER_SEC;
        printf("%d\tNaive\t0\t%f\n", N, naive_time);

        // Blocked Runs
        for (int b = 0; b < num_blocks; b++) {
            int block_size = blocks[b];
            
            start = clock();
            matmul_blocked(A, B, C, N, block_size);
            end = clock();
            double blocked_time = ((double)(end - start)) / CLOCKS_PER_SEC;
            printf("%d\tBlocked\t%d\t%f\n", N, block_size, blocked_time);
        }

        free_matrix(A, N);
        free_matrix(B, N);
        free_matrix(C, N);
    }

    return 0;
}
