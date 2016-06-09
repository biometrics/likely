#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <likely.h>

int main(int argc, char *argv[])
{
    if (argc != 5) {
        puts("Usage:\n\t"
                 "eigenfaces_model_translator <inference_file> <mean> <evecs> <object_file>\n"
             "\n"
             "Example:\n\t"
                 "eigenfaces_model_translator share/likely/eigenfaces_api/eigenfaces-api.lisp data/demo/lfwa_grayscale_mean.lm data/demo/lfwa_grayscale_evecs.lm model_translation_api.o\n");
        return EXIT_FAILURE;
    }

    // Reading model files...
    const likely_const_mat mean = likely_read(argv[2], likely_guess_file_type(argv[2]), likely_void);
    likely_ensure(mean, "failed to read mean file: %s", argv[2]);
    const likely_const_mat evecs = likely_read(argv[3], likely_guess_file_type(argv[3]), likely_void);
    likely_ensure(evecs, "failed to read evecs file: %s", argv[3]);

    // Constructing model translation environment...
    likely_const_mat output = NULL;
    const likely_file_type object_file_type = likely_guess_file_type(argv[4]);
    struct likely_settings settings = likely_default_settings(object_file_type, false);
    likely_const_env env = likely_standard(settings);
    likely_static((likely_env) env, &output, object_file_type, NULL);
    likely_define("mean", mean, &env);
    likely_define("evecs", evecs, &env);

    // Compiling inference algorithm...
    likely_read_lex_parse_and_eval(argv[1], &env);
    likely_release_env(env);
    likely_ensure(output, "failed to compile: %s", argv[1]);

    // Saving object file...
    const bool write_success = likely_write(output, argv[4]);
    likely_ensure(write_success, "failed to write: %s", argv[4]);

    // Cleaning up...
    likely_release_mat(evecs);
    likely_release_mat(mean);

    return EXIT_SUCCESS;
}
