#ifndef __TEST_H
#define __TEST_H

#include <opencv2/core/core.hpp>
#include "likely.h"

namespace likely
{

struct Test
{
    int run() const;

protected:
    virtual const char *function() const = 0;
    virtual cv::Mat computeBaseline(const cv::Mat &src) const = 0;
    virtual std::vector<likely_hash> types() const
    {
        std::vector<likely_hash> types;
//        types.push_back(likely_hash_i16);
//        types.push_back(likely_hash_i32);
//        types.push_back(likely_hash_i64);
        types.push_back(likely_hash_f32);
//        types.push_back(likely_hash_f64);
        return types;
    }

private:
    struct Speed
    {
        int iterations;
        double Hz;
        Speed() : iterations(-1), Hz(-1) {}
        Speed(int iter, clock_t startTime, clock_t endTime)
            : iterations(iter), Hz(double(iter) / (endTime-startTime)) {}
    };

    void testCorrectness(UnaryFunction f, const cv::Mat &src, bool parallel) const;
    Speed testBaselineSpeed(const cv::Mat &src) const;
    Speed testLikelySpeed(UnaryFunction f, const cv::Mat &src, bool parallel) const;
    void printSpeedup(const Speed &baseline, const Speed &likely, const char *mode) const;
};

} // namespace likely

// Allows us to remove three lines of code from each test file
using namespace cv;
using namespace likely;
using namespace std;

#endif // __TEST_H
