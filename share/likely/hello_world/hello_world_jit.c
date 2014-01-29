#include <likely.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
	char* imageName;
	char* outputFilename;
	char* filter;

	if (argc == 1) {
		imageName = "../../../data/misc/lenna.tiff";
		outputFilename = "dark_lenna.png";
		filter = "(kernel (a) (/ a 2))";
	}
	else if (argc == 4) {
		imageName = argv[1];
		outputFilename = argv[3];
		char* filterFilename = argv[2];

		FILE* fp = fopen(filterFilename, "rb");
		
		if (!fp) {
			printf("Failed to read filter!\n");
			return -1;
		}

		fseek(fp, 0, SEEK_END);
		long size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		filter = malloc(size);
		fread(filter, 1, size, fp);
		filter[size] = 0;
	}
	else {
		printf("Usage:\n");
		printf("\thello_world_jit");
		printf("\thello_world_jit <imagepath> <filterpath> <outputpath>");
		return -1;
	}

	printf("Reading input image...\n");
	likely_matrix lenna = likely_read(imageName);
	if (lenna) {
		printf("Width: %zu\nHeight: %zu\n", lenna->columns, lenna->rows);
	}
	else {
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
	likely_write(dark_lenna, outputFilename);

	printf("Releasing data...\n");
	likely_release(lenna);
	likely_release(dark_lenna);

	printf("Done!\n");
	return 0;
}
