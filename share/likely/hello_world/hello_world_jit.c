#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*!
 * \page hello_world_jit Hello World JIT
 * \brief Documentation and source code for the \c hello_world_jit application.
 *
 * This application takes three parameters:
\verbatim
$ hello_world_jit <input_image> <function> <output_image>
\endverbatim
 *
 * \c hello_world_jit calls \c \<function\> on \c \<input_image\> and saves the result to \c \<output_image\>.
 * Note that the provided \c \<function\> should only take one parameter, the \c \<input_image\>.
 *
 * This application is a good entry point for learning about the Likely API.
 * Its source code in <tt>share/likely/hello_world/hello_world_jit.c</tt> is reproduced below:
 * \snippet share/likely/hello_world/hello_world_jit.c hello_world_jit implementation.
 * Your next reading assignment is \ref console_interpreter_compiler.
 */

//! [hello_world_jit implementation.]
#include <likely.h>

int main(int argc, char *argv[])
{
    if (argc != 4) {
        puts("Usage:\n\t"
                 "hello_world_jit <input_image> <function> <output_image>\n"
             "\n"
             "Example:\n\t"
                 "hello_world_jit data/misc/lenna.tiff share/likely/hello_world/hello-world-jit.lisp dark_lenna.png\n");
        return EXIT_FAILURE;
    }

    puts("Reading input image...");
    const likely_const_mat input = likely_read(argv[1], likely_file_guess, likely_image);
    likely_ensure(input, "failed to read: %s", argv[1]);
    printf("Dimensions: %ux%u\n", input->columns, input->rows);

    puts("Reading source code...");
    FILE *const file = fopen(argv[2], "rb");
    likely_ensure(file, "failed to open for reading: %s", argv[2]);
    fseek(file, 0L, SEEK_END);
    const long file_size = ftell(file);
    rewind(file);
    char *source_code = (char *) malloc(file_size+1);
    likely_ensure(fread(source_code, file_size, 1, file),
                  "failed to read: %s", argv[2]);
    source_code[file_size] = 0;
    fclose(file);

    puts("Creating a JIT compiler environment...");
    const likely_env parent = likely_standard(likely_jit(false), NULL);

    puts("Compiling source code...");
    const likely_const_env env = likely_lex_parse_and_eval(source_code, likely_file_lisp, parent);
    likely_mat (*hello_world)(likely_const_mat) = likely_function(env->expr);
    likely_ensure(hello_world, "failed to compile source code");
    free(source_code);
    source_code = NULL;

    puts("Calling compiled function...");
    const likely_const_mat output = hello_world(input);

    puts("Writing output image...");
    const likely_const_mat write_success = likely_write(output, argv[3]);
    likely_ensure(write_success, "failed to write: %s", argv[3]);
    likely_release_mat(write_success);

    puts("Cleaning up...");
    likely_release_mat(output);
    likely_release_env(env);
    likely_release_env(parent);
    likely_release_mat(input);

    puts("Done!");
    likely_shutdown();

    return EXIT_SUCCESS;
}
//! [hello_world_jit implementation.]
