#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <likely.h>

int main(int argc, char *argv[])
{
    if (argc != 4) {
        puts("Usage:\n\t"
                 "hello_world_jit <source_file> <training_parameter> <bitcode_file>\n"
             "\n"
             "Example:\n\t"
                 "hello_world_jit share/likely/hello_world/hello-world-trained.lisp 2 hello_world_trained_lisp.bc\n");
        return EXIT_FAILURE;
    }

    puts("Reading source code...");
    const likely_file_type file_type = likely_guess_file_type(argv[1]);
    const likely_const_mat source_file = likely_read(argv[1], file_type, likely_void);
    likely_ensure(source_file, "failed to open for reading: %s", argv[1]);

    puts("Creating a JIT compiler environment...");
    const likely_env parent = likely_standard(likely_jit(false), NULL, likely_file_void);

    puts("Compiling source code...");
    const likely_const_env env = likely_lex_parse_and_eval(source_file->data, file_type, parent);
//    likely_env (*hello_world_train)(likely_const_mat) = likely_function(env->expr);
//    likely_ensure(hello_world_train, "failed to compile training function");

    puts("Cleaning up...");
    likely_release_env(env);
    likely_release_env(parent);
    likely_release_mat(source_file);

    return EXIT_SUCCESS;
}
