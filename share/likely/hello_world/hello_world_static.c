#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <likely/runtime.h>

// Provided by a call to the static compiler:
// $ like hello_world_div2.ll hello_world_div2.o
likely_const_mat hello_world_div2(const likely_const_mat m);

int main()
{
    const int elements = 1000;
    likely_const_mat input = likely_new(likely_matrix_f32, 1, elements, 1, 1, NULL);
    likely_const_mat str = likely_type_to_string(input->type);
    printf("Input type: %s\n", str->data);
    likely_release(str);

    printf("Initializing input...\n");
    srand((unsigned int) time(NULL));
    for (int i=0; i<elements; i++)
        ((float*)input->data)[i] = (float) rand();

    printf("Computing output...\n");
    likely_const_mat output = hello_world_div2(input);

    printf("Checking output...\n");
    for (int i=0; i<elements; i++)
        if (((float*)output->data)[i] != ((float*)input->data)[i] / 2) {
            printf("Unexpected result:\n\tindex: %d\n\tinput: %g\n\texpected: %g\n\tactual: %g",
                   i, ((float*)input->data)[i], ((float*)input->data)[i]/2, ((float*)output->data)[i]);
            abort();
        }

    printf("Releasing data...\n");
    likely_release(input);
    likely_release(output);

    printf("Done!\n");
    return 0;
}
