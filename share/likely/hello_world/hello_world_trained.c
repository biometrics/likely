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
                 "hello_world_jit <source_file> <training_parameter> <object_file>\n"
             "\n"
             "Example:\n\t"
                 "hello_world_jit share/likely/hello_world/hello-world-trained.lisp 2 hello_world_trained_lisp.bc\n");
        return EXIT_FAILURE;
    }

    puts("Reading source code...");
    const likely_file_type source_file_type = likely_guess_file_type(argv[1]);
    const likely_const_mat source_file = likely_read(argv[1], source_file_type, likely_void);

    puts("Reading training parameter...");
    likely_set_global("training-parameter", likely_compute(argv[2]));

    puts("Compiling source code...");
    likely_mat output = NULL;
    const likely_file_type object_file_type = likely_guess_file_type(argv[3]);
    const struct likely_settings settings = likely_default_settings(object_file_type, false);
    const likely_env parent = likely_standard(settings, &output, object_file_type);
    likely_release_env(likely_lex_parse_and_eval(source_file->data, source_file_type, parent));
    likely_release_env(parent);
    likely_ensure(output, "failed to compile: %s", argv[1]);

    puts("Saving bitcode...");
    const bool write_success = likely_write(output, argv[3]);
    likely_ensure(write_success, "failed to write: %s", argv[3]);

    puts("Cleaning up...");
    likely_release_mat(source_file);

    return EXIT_SUCCESS;
}
