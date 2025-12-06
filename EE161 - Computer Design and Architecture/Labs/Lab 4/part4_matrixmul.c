#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_N 512

// Allocate 2D matrix
double **alloc_matrix(int n) {
    double **m = malloc(n * sizeof(double *));
    m[0] = malloc(n * n * sizeof(double));
    for (int i = 1; i < n; i++)
        m[i] = m[0] + i * n;
    return m;
}

// Initialize matrix with random values
void init_matrix(double **m, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            m[i][j] = (double)(rand() % 10);
}

// Free matrix
void free_matrix(double **m) {
    free(m[0]);
    free(m);
}

// Naive triple-loop matrix multiplication
void matmul_naive(double **A, double **B, double **C, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            double sum = 0.0;
            for (int k = 0; k < n; k++)
                sum += A[i][k] * B[k][j];
            C[i][j] = sum;
        }
}

// Blocked matrix multiplication
void matmul_blocked(double **A, double **B, double **C, int n, int block) {
    for (int ii = 0; ii < n; ii += block)
        for (int jj = 0; jj < n; jj += block)
            for (int kk = 0; kk < n; kk += block)
                for (int i = ii; i < ii + block && i < n; i++)
                    for (int j = jj; j < jj + block && j < n; j++) {
                        double sum = C[i][j];
                        for (int k = kk; k < kk + block && k < n; k++)
                            sum += A[i][k] * B[k][j];
                        C[i][j] = sum;
                    }
}

double wall_time() {
    return (double)clock() / CLOCKS_PER_SEC;
}

int main() {
    srand(0);
    int sizes[] = {256};
    int blocks[] = {16};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    int num_blocks = sizeof(blocks) / sizeof(blocks[0]);

    printf("Matrix Multiplication Performance\n");
    printf("N\tBlock\tNaive(s)\tBlocked(s)\n");

    for (int s = 0; s < num_sizes; s++) {
        int N = sizes[s];

        double **A = alloc_matrix(N);
        double **B = alloc_matrix(N);
        double **C = alloc_matrix(N);

        init_matrix(A, N);
        init_matrix(B, N);

        // Naive multiplication
        double start = wall_time();
        matmul_naive(A, B, C, N);
        double naive_time = wall_time() - start;

        for (int b = 0; b < num_blocks; b++) {
            int block = blocks[b];

            // Reinitialize C
            for (int i = 0; i < N; i++)
                for (int j = 0; j < N; j++)
                    C[i][j] = 0.0;

            double start_b = wall_time();
            matmul_blocked(A, B, C, N, block);
            double blocked_time = wall_time() - start_b;

            printf("%d\t%d\t%.4f\t\t%.4f\n", N, block, naive_time, blocked_time);
        }

        free_matrix(A);
        free_matrix(B);
        free_matrix(C);
    }

    return 0;
}
