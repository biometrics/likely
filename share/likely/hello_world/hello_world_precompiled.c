#include <stdio.h>
#include <stdlib.h>
#include <likely.h>

// <bitcode> provided by a call to the static compiler:
// $ likely share/likely/hello_world/hello-world-compiled.lisp hello_world.bc

int main(int argc, char *argv[])
{
    if (argc != 5) {
        puts("Usage:\n\t"
                 "hello_world_precompiled <input_image> <bitcode> <symbol> <output_image>\n"
             "\n"
             "Example:\n\t"
                 "hello_world_precompiled data/misc/lenna.tiff hello_world.bc hello_world dark_lenna.png\n");
        return EXIT_FAILURE;
    }

    puts("Reading input image...");
    const likely_const_mat input = likely_read(argv[1], likely_file_guess, likely_image);

    puts("Reading bitcode...");
    const likely_const_mat bitcode = likely_read(argv[2], likely_file_bitcode, likely_void);

    puts("Compiling function...");
    const struct likely_settings settings = likely_default_settings(likely_file_void, false);
    const likely_const_env env = likely_standard(settings);
    likely_mat (*hello_world)(likely_const_mat) = likely_jit((likely_env) env, bitcode, argv[3]);
    likely_ensure(hello_world, "failed to compile bitcode function %s:", argv[3]);

    puts("Calling compiled function...");
    const likely_const_mat output = hello_world(input);

    puts("Writing output image...");
    const bool write_success = likely_write(output, argv[4]);
    likely_ensure(write_success, "failed to write: %s", argv[4]);

    puts("Cleaning up...");
    likely_release_mat(output);
    likely_release_env(env);
    likely_release_mat(bitcode);
    likely_release_mat(input);

    return EXIT_SUCCESS;
}
