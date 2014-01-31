#include <likely.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    char *inputImage;
    char *outputImage;
    char *filter;

    if (argc == 1) {
#ifdef _WIN32 // TODO: Windows binaries should end up in build/bin
        imageName = "../../../data/misc/lenna.tiff";
#else
        inputImage = "../data/misc/lenna.tiff";
#endif
        outputImage = "dark_lenna.png";
        filter = "(kernel (a) (/ a 2))";
    } else if (argc == 4) {
        inputImage = argv[1];
        outputImage = argv[3];

        FILE* fp = fopen(argv[2], "rb");
        if (!fp) {
            printf("Failed to open filter!\n");
            return -1;
        }

        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        filter = malloc(size);
        long size_read = fread(filter, 1, size, fp);
        if (size_read != size) {
            printf("Failed to read filter!\n");
            return -1;
        }
        filter[size] = 0;
    } else {
        printf("Usage:\n");
        printf("\thello_world_jit\n");
        printf("\thello_world_jit <input_image> <filter_file> <output_image>\n");
        return -1;
    }

    printf("Reading input image...\n");
    likely_matrix lenna = likely_read(inputImage);
    if (lenna) {
        printf("Width: %zu\nHeight: %zu\n", lenna->columns, lenna->rows);
    } else {
        printf("Failed to read!\n");
        return -1;
    }

    if (lenna->rows == 0 || lenna->columns == 0) {
        printf("Image width or height is zero!\n");
        return -1;
    }

    printf("Parsing abstract syntax tree...\n");
    likely_ast ast = likely_ast_from_string(filter);

    likely_assert(ast->num_atoms == 1, "expected a single expression");

    printf("Compiling source code...\n");
    likely_function darken = likely_compile(ast->atoms[0]);
    likely_release_ast(ast);
    if (!darken) {
        printf("Failed to compile!\n");
        return -1;
    }

    printf("Calling compiled function...\n");
    likely_matrix dark_lenna = darken(lenna);
    if (!dark_lenna) {
        printf("Failed to execute!\n");
        return -1;
    }

    printf("Writing output image...\n");
    likely_write(dark_lenna, outputImage);

    printf("Releasing data...\n");
    likely_release(lenna);
    likely_release(dark_lenna);

    printf("Done!\n");
    return 0;
}
