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
    const likely_const_mat training_parameter = likely_compute(argv[2]);
    likely_ensure(training_parameter, "failed to compute training parameter: %s", argv[2]);

    puts("Constructing training environment...");
    likely_mat output = NULL;
    const likely_file_type object_file_type = likely_guess_file_type(argv[3]);
    struct likely_settings settings = likely_default_settings(object_file_type, false);
    settings.runtime_only = true;
    likely_const_env parent = likely_standard(settings, &output, object_file_type);

    {
        char source[128];
        if (training_parameter->type & likely_multi_dimension) {
            const likely_const_mat trainingParameterType = likely_type_to_string(training_parameter->type);
            snprintf(source, 128, "(= training-parameter (%s %zu))", trainingParameterType->data, (uintptr_t)training_parameter);
            likely_release_mat(trainingParameterType);
        } else {
            snprintf(source, 128, "(= training-parameter %g)", likely_get_element(training_parameter, 0, 0, 0, 0));
        }
        const likely_const_env env = likely_lex_parse_and_eval(source, likely_file_lisp, parent);
        likely_ensure(env, "failed to construct training environment");
        likely_release_env(parent);
        parent = env;
    }

    puts("Compiling source code...");
    likely_release_env(likely_lex_parse_and_eval(source_file->data, source_file_type, parent));
    likely_release_env(parent);
    likely_ensure(output, "failed to compile: %s", argv[1]);

    puts("Saving bitcode...");
    const bool write_success = likely_write(output, argv[3]);
    likely_ensure(write_success, "failed to write: %s", argv[3]);

    puts("Cleaning up...");
    likely_release_mat(training_parameter);
    likely_release_mat(source_file);

    return EXIT_SUCCESS;
}
