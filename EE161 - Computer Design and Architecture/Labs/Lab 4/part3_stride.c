#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 64 MB total array size as per instructions
#define ARRAY_SIZE_BYTES (64 * 1024 * 1024)
#define ELEMENT_SIZE sizeof(int)
#define NUM_ELEMENTS (ARRAY_SIZE_BYTES / ELEMENT_SIZE)

int main() {
    int *array = (int *)malloc(ARRAY_SIZE_BYTES);
    if (!array) {
        perror("Memory allocation failed");
        return 1;
    }

    // Initialize the array
    for (size_t i = 0; i < NUM_ELEMENTS; i++) {
        array[i] = 1;
    }

    // Strides specified in instructions: 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192 bytes
    int strides[] = {4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
    int num_strides = sizeof(strides) / sizeof(strides[0]);

    printf("Stride(bytes)\tTime(s)\n");

    for (int s = 0; s < num_strides; s++) {
        int stride_bytes = strides[s];
        int stride_elements = stride_bytes / ELEMENT_SIZE;
        
        // Ensure stride is at least 1 element
        if (stride_elements < 1) stride_elements = 1;

        // To ensure we touch the same number of elements or walk the whole array?
        // "walks through a large 1D array using a configurable stride"
        // Usually, we want to touch the whole array to see the cache effects.
        // If we just skip elements, we do fewer accesses. 
        // Standard stride benchmark: Read array[0], array[stride], array[2*stride]...
        // But to keep work constant, we might repeat. 
        // However, the instructions imply just "run the program with strides".
        // Let's assume we iterate through the entire array once with that stride.
        // Wait, if stride increases, number of accesses decreases. 
        // "demonstrate performance changes as stride crosses cache line size"
        // If we do fewer accesses, time drops naturally. We want time per access or total time for fixed number of accesses.
        // Usually, we do: for (i=0; i < STEPS; i++) sum += array[(i * stride) % SIZE];
        // But let's look at the previous `part3_stride.c` logic.
        
        // Previous logic:
        // for (int offset = 0; offset < stride_elements; offset++) {
        //    for (size_t i = offset; i < NUM_ELEMENTS; i += stride_elements) { ... }
        // }
        // This visits EVERY element in the array exactly once, just in a strided order.
        // This keeps the total work constant (N accesses), isolating the effect of memory pattern.
        // I will stick to this logic.

        volatile int sum = 0;
        clock_t start = clock();

        for (int offset = 0; offset < stride_elements; offset++) {
            for (size_t i = offset; i < NUM_ELEMENTS; i += stride_elements) {
                sum += array[i];
            }
        }

        clock_t end = clock();
        double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

        printf("%d\t%f\n", stride_bytes, elapsed);
    }

    free(array);
    return 0;
}
