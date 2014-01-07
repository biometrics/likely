#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <likely/likely_runtime.h>

// Provided by a call to the static compiler:
// $ like 'div(arg(0), 2)' hello_world_div2 f32 hello_world_div2.o
likely_mat hello_world_div2(likely_const_mat m);

int main()
{
    const int elements = 1000;
    likely_mat input = likely_new(likely_type_f32, 1, elements, 1, 1, NULL, 0);

    srand(time(NULL));
    for (int i=0; i<elements; i++)
        ((float*)input->data)[i] = rand();

    likely_mat output = hello_world_div2(input);
    for (int i=0; i<elements; i++)
        if (((float*)output->data)[i] != ((float*)input->data)[i] / 2) {
            printf("Unexpected result! Input: %g, Expected Ouput: %g, Actual Output: %g",
                   ((float*)input->data)[i],
                   ((float*)input->data)[i]/2,
                   ((float*)output->data)[i]);
            abort();
        }

    likely_release(input);
    likely_release(output);
    return 0;
}
