#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <likely.h>

// Provided by a call to the static compiler:
// $ likely share/likely/hello_world/hello-world-compiled.lisp hello_world.o
likely_const_mat hello_world(const likely_const_mat m);

int main(int argc, char *argv[])
{
    if (argc != 3) {
        puts("Usage:\n\t"
                 "hello_world_static <input_image> <output_image>\n"
             "\n"
             "Example:\n\t"
                 "hello_world_static data/misc/lenna.tiff dark_lenna.png\n");
        return EXIT_FAILURE;
    }

    puts("Reading input image...");
    const likely_const_mat input = likely_read(argv[1], likely_file_guess, likely_image);
    likely_ensure(input, "failed to read: %s", argv[1]);
    printf("Dimensions: %ux%u\n", input->columns, input->rows);

    puts("Calling compiled function...");
    const likely_const_mat output = hello_world(input);

    puts("Writing output image...");
    const bool write_success = likely_write(output, argv[2]);
    likely_ensure(write_success, "failed to write: %s", argv[2]);

    puts("Cleaning up...");
    likely_release_mat(output);
    likely_release_mat(input);

    return EXIT_SUCCESS;
}
