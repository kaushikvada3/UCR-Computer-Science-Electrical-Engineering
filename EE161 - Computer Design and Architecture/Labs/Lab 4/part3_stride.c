#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ARRAY_SIZE (64 * 1024 * 1024)  // 64 MB total
#define ELEMENT_SIZE sizeof(int)
#define NUM_ELEMENTS (ARRAY_SIZE / ELEMENT_SIZE)

int main() {
    int *array = malloc(NUM_ELEMENTS * sizeof(int));
    if (!array) {
        perror("Memory allocation failed");
        return 1;
    }

    // Initialize the array to avoid lazy allocation and compiler optimization
    for (size_t i = 0; i < NUM_ELEMENTS; i++) {
        array[i] = (int)i;
    }

    int strides[] = {4};
    int num_strides = sizeof(strides) / sizeof(strides[0]);

    printf("Stride (bytes)\tTime (seconds)\n");

    for (int s = 0; s < num_strides; s++) {
        int stride_bytes = strides[s];
        int stride_elements = stride_bytes / ELEMENT_SIZE;
        if (stride_elements == 0) stride_elements = 1;

        volatile int sum = 0;
        clock_t start = clock();

        // For each offset in stride range
        for (int offset = 0; offset < stride_elements; offset++) {
            // Visit every element in that offset group: 0, 4, 8,... then 1,5,9...
            for (size_t i = offset; i < NUM_ELEMENTS; i += stride_elements) {
                sum += array[i];
            }
        }

        clock_t end = clock();
        double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

        printf("%6d\t\t%.6f\n", stride_bytes, elapsed);
    }

    free(array);
    return 0;
}
