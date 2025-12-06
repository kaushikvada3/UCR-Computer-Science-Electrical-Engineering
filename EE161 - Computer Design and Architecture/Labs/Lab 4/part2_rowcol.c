#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    // Sizes specified in instructions: 1024, 2048, 4096, 8192
    int sizes[] = {1024, 2048, 4096, 8192};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    printf("Size\tRowTime\tColTime\n");

    for (int s = 0; s < num_sizes; s++) {
        int N = sizes[s];
        
        // Allocate matrix dynamically (N*N ints)
        // 8192 * 8192 * 4 bytes = 256 MB
        int *A = (int *)malloc((size_t)N * N * sizeof(int));
        if (!A) {
            perror("Allocation failed");
            return 1;
        }

        // Initialize to avoid compiler optimizing away loops (though unlikely with volatile or complex logic, simple init is safer)
        for (long long i = 0; i < (long long)N * N; i++) {
            A[i] = 1;
        }

        clock_t start, end;
        double row_time, col_time;

        // Row-major traversal
        start = clock();
        long long sum1 = 0;
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                sum1 += A[i * N + j];
            }
        }
        end = clock();
        row_time = ((double) (end - start)) / CLOCKS_PER_SEC;

        // Column-major traversal
        start = clock();
        long long sum2 = 0;
        for (int j = 0; j < N; j++) {
            for (int i = 0; i < N; i++) {
                sum2 += A[i * N + j];
            }
        }
        end = clock();
        col_time = ((double) (end - start)) / CLOCKS_PER_SEC;

        printf("%d\t%f\t%f\n", N, row_time, col_time);

        free(A);
    }

    return 0;
}