#ifndef __TEST_H
#define __TEST_H

#include <opencv2/core/core.hpp>

namespace likely
{

struct Test
{
    int run() const;

private:
    virtual const char *function() const = 0;
    virtual cv::Mat computeBaseline(const cv::Mat &src) const = 0;
};

} // namespace likely

using namespace cv;
using namespace likely;
using namespace std;

#endif // __TEST_H
