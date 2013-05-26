#include <ctime>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>

#include "test.h"

#define LIKELY_ERROR_TOLERANCE 0.0001
#define LIKELY_TEST_SECONDS 1

static Matrix matrixFromMat(const Mat &mat)
{
    likely_assert(mat.isContinuous(), "Continuous data required");

    Matrix m;
    m.data = mat.data;
    m.channels = mat.channels();
    m.columns = mat.cols;
    m.rows = mat.rows;
    m.frames = 1;

    switch (mat.depth()) {
      case CV_8U:  m.hash = likely_hash_u8;  break;
      case CV_8S:  m.hash = likely_hash_i8;  break;
      case CV_16U: m.hash = likely_hash_u16; break;
      case CV_16S: m.hash = likely_hash_i16; break;
      case CV_32S: m.hash = likely_hash_i32; break;
      case CV_32F: m.hash = likely_hash_f32; break;
      case CV_64F: m.hash = likely_hash_f64; break;
      default:     likely_assert(false, "Unsupported matrix depth");
    }

    return m;
}

static int getOpenCVType(likely_hash type, int channels = 1)
{
    int depth = -1;
    switch (type) {
      case likely_hash_u8:  depth = CV_8U;  break;
      case likely_hash_i8:  depth = CV_8S;  break;
      case likely_hash_u16: depth = CV_16U; break;
      case likely_hash_i16: depth = CV_16S; break;
      case likely_hash_i32: depth = CV_32S; break;
      case likely_hash_f32: depth = CV_32F; break;
      case likely_hash_f64: depth = CV_64F; break;
      default:              likely_assert(false, "Unsupported matrix depth");
    }

    return CV_MAKETYPE(depth, channels);
}

static Mat matrixToMat(const Matrix &m)
{
    return Mat(m.rows, m.columns, getOpenCVType(m.type(), m.channels), m.data);
}

static Mat generateData(int rows, int columns, likely_hash type)
{
    Mat m(rows, columns, getOpenCVType(type));
    randu(m, 0, 255);
    return m;
}

int Test::run() const
{
    setbuf(stdout, NULL);
    printf("%s\n", function());
    UnaryFunction f = makeUnaryFunction(function());

    for (likely_hash type : types()) {
        // Generate input matrix
        Mat src = generateData(100, 100, type);

        // Test correctness
        testCorrectness(f, src, false);
        testCorrectness(f, src, true);

        // Test speed
        Speed baseline = testBaselineSpeed(src);
        printSpeedup(baseline, testLikelySpeed(f, src, false), "Serial  ");
        printSpeedup(baseline, testLikelySpeed(f, src, true),  "Parallel");
    }

    return 0;
}

void Test::testCorrectness(UnaryFunction f, const Mat &src, bool parallel) const
{
    Mat dstOpenCV = computeBaseline(src);
    Matrix srcLikely = matrixFromMat(src).setParallel(parallel);
    Matrix dstLikely;
    f(&srcLikely, &dstLikely);

    Mat errorMat;
    threshold(abs(matrixToMat(dstLikely) - dstOpenCV), errorMat, LIKELY_ERROR_TOLERANCE, 1, THRESH_BINARY);
    const double errors = norm(errorMat, NORM_L1);
    likely_assert(errors == 0, "Test for %s differs in %g locations", function(), errors);
}

Test::Speed Test::testBaselineSpeed(const Mat &src) const
{
    clock_t startTime, endTime;
    int iter = 0;
    startTime = endTime = clock();
    while ((endTime-startTime) / CLOCKS_PER_SEC < LIKELY_TEST_SECONDS) {
        computeBaseline(src);
        endTime = clock();
        iter++;
    }
    return Test::Speed(iter, startTime, endTime);
}

Test::Speed Test::testLikelySpeed(UnaryFunction f, const Mat &src, bool parallel) const
{
    Matrix srcLikely = matrixFromMat(src).setParallel(parallel);

    clock_t startTime, endTime;
    int iter = 0;
    startTime = endTime = clock();
    while ((endTime-startTime) / CLOCKS_PER_SEC < LIKELY_TEST_SECONDS) {
        Matrix dstLikely;
        f(&srcLikely, &dstLikely);
        endTime = clock();
        iter++;
    }
    return Test::Speed(iter, startTime, endTime);
}

void Test::printSpeedup(const Speed &baseline, const Speed &likely, const char *mode) const
{
    printf("  %s: %gx (~= %d/%d)\n", mode, likely.Hz / baseline.Hz, likely.iterations, baseline.iterations);
}
