#define CL_ARRAY_IMPLEMENTATION
#include <stdio.h>
#include "arr.h"
#include "assertf.h"

typedef struct {
    int x, y;
} Position;

int main() {
    Position *positions = CL_ARRAY_INIT;

    size_t len;
    int items[] = {3, 4, 5, 6, 7, 8, 23, 5};

    cl_arr_push(positions, ((Position){ 10, 15 }));
    cl_arr_push(positions, ((Position){ 10, 30 }));

    for (size_t i = 0; i < sizeof(items) / sizeof(*items); ++i) {
        cl_arr_push(positions, ((Position){ items[i], items[i] + i }));
    }

    len = cl_arr_len(positions);

    printf("size: %ld\n", len);

    assertf(len == (sizeof(items) / sizeof(*items)) + 2, "invalid length");

    cl_arr_pop(positions);

    len = cl_arr_len(positions);

    printf("size: %ld\n", len);

    assertf(len == (sizeof(items) / sizeof(*items)) + 1, "invalid length");

    cl_arr_free(positions);

    int *numbers = CL_ARRAY_INIT;

    int a = 0;
    int b = 1;
    int t = 0;

    for (size_t i = 0; i < 10; i++) {
        t = b;
        b = a + b;
        a = t;

        cl_arr_push(numbers, a);
    }

    for (size_t i = 0; i < cl_arr_len(numbers); ++i) {
        if (i > 0) printf(", ");

        printf("%d", numbers[i]);
    }

    printf("\n");

    cl_arr_free(numbers);

    float *weights = CL_ARRAY_INIT;

    cl_arr_push(weights, 37.56);
    cl_arr_push(weights, 45.78);
    cl_arr_push(weights, 69.33);

    printf("[");
    for (size_t i = 0; i < cl_arr_len(weights); ++i)
    {
        if (i > 0) printf(", ");

        printf("%f", weights[i]);
    }
    printf("]\n");

    cl_arr_u_remove(weights, 1);

    printf("[");
    for (size_t i = 0; i < cl_arr_len(weights); ++i)
    {
        if (i > 0) printf(", ");

        printf("%f", weights[i]);
    }
    printf("]\n");

    cl_arr_u_remove(weights, cl_arr_len(weights) - 1);

    printf("[");
    for (size_t i = 0; i < cl_arr_len(weights); ++i)
    {
        if (i > 0) printf(", ");

        printf("%f", weights[i]);
    }
    printf("]\n");

    float last_weight = cl_arr_pop(weights);
    printf("last weight: %f, resulting length: %ld\n", last_weight, cl_arr_len(weights));

    cl_arr_free(weights);

    return 0;
}
