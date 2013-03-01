#include <opencv2/core/core.hpp>
#include "likely.h"

using namespace cv;

int main(int argc, char *argv[])
{
    (void) argc; (void) argv;

    Mat m(1000, 1000, CV_32FC1);
    randu(m, 0, 255);

    likely_unary_function function = likely_make_unary_function("madd(2,3)");
    (void) function;
    return 0;
}
