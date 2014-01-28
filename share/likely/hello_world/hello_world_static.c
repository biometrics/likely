#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <likely/likely_runtime.h>

// Provided by a call to the static compiler:
// $ like 'div(arg(0), 2)' hello_world_div2 f32 hello_world_div2.o
likely_matrix hello_world_div2(const likely_matrix m);

int main()
{
    const int elements = 1000;
    likely_matrix input = likely_new(likely_type_f32, 1, elements, 1, 1, NULL, 0);
    printf("Input type: %s\n", likely_type_to_string(input->type));

    printf("Initializing input...\n");
    srand((unsigned int) time(NULL));
    for (int i=0; i<elements; i++)
        ((float*)input->data)[i] = (float) rand();

    printf("Computing output...\n");
    likely_matrix output = hello_world_div2(input);

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
