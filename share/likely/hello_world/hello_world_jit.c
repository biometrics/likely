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
                 "hello_world_jit data/misc/lenna.tiff 'a :-> { dst := a.imitate (dst a) :=> (<- dst (/ a (a.type 2))) }' dark_lenna.png\n");
        return EXIT_FAILURE;
    }

    puts("Initializing compiler...");
    likely_initialize(3, 0, true, false);

    puts("Reading input image...");
    const likely_const_mat input = likely_read(argv[1], likely_file_guess);
    likely_assert(input, "failed to read: %s", argv[1]);
    printf("Width: %u\nHeight: %u\n", input->columns, input->rows);

    puts("Parsing function...");
    const likely_ast ast = likely_lex_and_parse(argv[2], likely_file_lisp);

    puts("Creating a JIT compiler environment...");
    const likely_env parent = likely_standard(NULL);

    puts("Compiling source code...");
    const likely_const_env env = likely_eval(ast, parent, NULL, NULL);
    likely_assert(env->expr, "failed to evaluate: %s", argv[2]);
    likely_mat (*function)(likely_const_mat) = likely_compile(env->expr, &input->type, 1);
    likely_assert(function, "failed to compile: %s", argv[2]);

    puts("Calling compiled function...");
    const likely_const_mat output = function(input);

    puts("Writing output image...");
    const likely_const_mat write_success = likely_write(output, argv[3]);
    likely_assert(write_success, "failed to write: %s", argv[3]);
    likely_release_mat(write_success);

    puts("Cleaning up...");
    likely_release_mat(output);
    likely_release_env(env);
    likely_release_env(parent);
    likely_release_ast(ast);
    likely_release_mat(input);

    puts("Done!");
    likely_shutdown();

    return EXIT_SUCCESS;
}
//! [hello_world_jit implementation.]
