#include <opencv2/core/core.hpp>
#include "likely.h"

using namespace cv;
using namespace likely;

int main(int argc, char *argv[])
{
    (void) argc; (void) argv;

    Mat m(1000, 1000, CV_32FC1);
    randu(m, 0, 255);

    UnaryFunction function = makeUnaryFunction("madd(2,3)");
    Matrix src(Matrix::f32), dst;
    function(&src, &dst);

    return 0;
}
