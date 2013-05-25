#include <ctime>
#include <iostream>

#include "test.h"

#define LIKELY_ERROR_TOLERANCE 0.0001
#define LIKELY_TEST_SECONDS 1

static Matrix matrixFromMat(const Mat &mat)
{
    if (!mat.isContinuous()) {
        fprintf(stderr, "ERROR - Continuous data required.\n");
        abort();
    }

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
      default:     fprintf(stderr, "ERROR - Unsupported matrix depth.\n"); abort();
    }

    return m;
}

static Mat matrixToMat(const Matrix &m)
{
    int depth = -1;
    switch (m.type()) {
      case likely_hash_u8:  depth = CV_8U;  break;
      case likely_hash_i8:  depth = CV_8S;  break;
      case likely_hash_u16: depth = CV_16U; break;
      case likely_hash_i16: depth = CV_16S; break;
      case likely_hash_i32: depth = CV_32S; break;
      case likely_hash_f32: depth = CV_32F; break;
      case likely_hash_f64: depth = CV_64F; break;
      default:              fprintf(stderr, "ERROR - Unsupported matrix depth.\n"); abort();
    }
    return Mat(m.rows, m.columns, CV_MAKETYPE(depth, m.channels), m.data);
}

int Test::run() const
{
    UnaryFunction f = makeUnaryFunction(function());

    // Generate input matrix
    Mat src(100, 100, CV_32FC1);
    randu(src, 0, 255);

    // Test correctness
    testCorrectness(f, src, false);
//    testCorrectness(f, src, true);

    // Test speed
    Speed baseline = testBaselineSpeed(src);
    printSpeedup(baseline, testLikelySpeed(f, src, false), "  Serial");
//    printSpeedup(baseline, testLikelySpeed(f, src, true),  "Parallel");

    return 0;
}

void Test::testCorrectness(UnaryFunction f, const Mat &src, bool parallel) const
{
    Mat dstOpenCV = computeBaseline(src);
    Matrix srcLikely = matrixFromMat(src).setParallel(parallel);
    Matrix dstLikely;
    f(&srcLikely, &dstLikely);

    const double errors = norm(abs(matrixToMat(dstLikely) - dstOpenCV) > LIKELY_ERROR_TOLERANCE, NORM_L1);
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
    Matrix dstLikely;

    clock_t startTime, endTime;
    int iter = 0;
    startTime = endTime = clock();
    while ((endTime-startTime) / CLOCKS_PER_SEC < LIKELY_TEST_SECONDS) {
        f(&srcLikely, &dstLikely);
        endTime = clock();
        iter++;
    }
    return Test::Speed(iter, startTime, endTime);
}

void Test::printSpeedup(const Speed &baseline, const Speed &likely, const char *mode) const
{
    printf("%s: %gx (~= %d/%d)\n", mode, likely.Hz / baseline.Hz, likely.iterations, baseline.iterations);
}
