#include <likely.h>
#include <stdio.h>

int main()
{
    printf("Reading input image...\n");
    likely_mat lenna = likely_read("../data/misc/lenna.tiff");
    if (lenna) {
        printf("Width: %zu\nHeight: %zu\n", lenna->columns, lenna->rows);
    } else {
        printf("Failed to read!\n");
        return -1;
    }

    printf("Parsing abstract syntax tree...\n");
    likely_ast ast = likely_ast_from_string("(kernel (a) (/ a 2))");
    likely_assert(ast.num_atoms == 1, "expected a single expression");

    printf("Compiling source code...\n");
    likely_function darken = likely_compile(ast.atoms[0]);
    if (!darken) {
        printf("Failed to compile!\n");
        return -1;
    }

    printf("Calling compiled function...\n");
    likely_mat dark_lenna = darken(lenna);
    if (!dark_lenna) {
        printf("Failed to execute!\n");
        return -1;
    }

    printf("Writing output image...\n");
    likely_write(dark_lenna, "dark_lenna.png");

    printf("Releasing data...\n");
    likely_release(lenna);
    likely_release(dark_lenna);

    printf("Done!\n");
    return 0;
}
