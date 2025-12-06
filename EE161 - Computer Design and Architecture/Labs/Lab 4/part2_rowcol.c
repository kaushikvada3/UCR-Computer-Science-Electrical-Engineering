#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 1024
int A[N][N];

int main() {
    clock_t start, end;
    double cpu_time_used;

    // Row-major
    start = clock();
    long long sum1 = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            sum1 += A[i][j];
        }
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Row-major time: %f sec\n", cpu_time_used);

    // Column-major
    start = clock();
    long long sum2 = 0;
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
            sum2 += A[i][j];
        }
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Column-major time: %f sec\n", cpu_time_used);

    return 0;
}