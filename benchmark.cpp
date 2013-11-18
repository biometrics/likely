/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2013 Joshua C. Klontz                                           *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <ctime>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <lua.hpp>

#include "likely.h"
#include "opencv.shim"

using namespace cv;
using namespace std;

#define LIKELY_ERROR_TOLERANCE 0.000001
#define LIKELY_TEST_SECONDS 1

// Optional arguments to run only a subset of the benchmarks
static bool        BenchmarkExamples  = false;
static string      BenchmarkFunction  = "";
static likely_type BenchmarkType      = likely_type_null;
static int         BenchmarkSize      = 0;
static int         BenchmarkExecution = -1;

static Mat generateData(int rows, int columns, likely_type type, double scaleFactor)
{
    static Mat m;
    if (!m.data) {
        m = imread("../img/Lenna.tiff");
        assert(m.data);
        cvtColor(m, m, CV_BGR2GRAY);
    }

    Mat n;
    resize(m, n, Size(columns, rows), 0, 0, INTER_NEAREST);
    n.convertTo(n, typeToDepth(type), scaleFactor);
    return n;
}

struct Test
{
    void run() const
    {
        if (!BenchmarkFunction.empty() && BenchmarkFunction != function()) return;

        for (likely_type type : types()) {
            if ((BenchmarkType != likely_type_null) && (BenchmarkType != type)) continue;

            for (int size : sizes()) {
                if ((BenchmarkSize != 0) && (BenchmarkSize != size)) continue;

                for (bool execution : executions()) {
                    if ((BenchmarkExecution != -1) && (BenchmarkExecution != execution)) continue;

                    // Generate input matrix
                    Mat src = generateData(size, size, type, scaleFactor());
                    likely_mat srcLikely = fromCvMat(src);
                    likely_set_parallel(&srcLikely->type, execution);
                    likely_function_1 f = (likely_function_1) likely_compile(function(), 1, srcLikely->type);
                    likely_release(srcLikely);

                    testCorrectness(f, src);
                    Speed baseline = testBaselineSpeed(src);
                    Speed likely = testLikelySpeed(f, src);
                    printf("%s \t%s \t%d \t%s \t%.2e\n", function(), likely_type_to_string(type), size, execution ? "Parallel" : "Serial", likely.Hz/baseline.Hz);
                }
            }
        }
    }

    static void runExample(const char *source)
    {
        static lua_State *L = likely_exec("");
        likely_exec(source, L);
        clock_t startTime, endTime;
        int iter = 0;
        startTime = endTime = clock();
        while ((endTime-startTime) / CLOCKS_PER_SEC < LIKELY_TEST_SECONDS) {
            likely_exec(source, L);
            endTime = clock();
            iter++;
        }
        Speed speed(iter, startTime, endTime);
        const size_t exampleStartPos = 3;
        const size_t exampleNameSize = string(source).find('\n') - exampleStartPos;
        printf("%s \t%.2e \n", string(source).substr(exampleStartPos, exampleNameSize).c_str(), speed.Hz);
    }

protected:
    virtual const char *function() const = 0;
    virtual cv::Mat computeBaseline(const cv::Mat &src) const = 0;
    virtual std::vector<likely_type> types() const
    {
        static std::vector<likely_type> types;
        if (types.empty()) {
            types.push_back(likely_type_u8);
            types.push_back(likely_type_u16);
            types.push_back(likely_type_i32);
            types.push_back(likely_type_f32);
            types.push_back(likely_type_f64);
        }
        return types;
    }

    virtual std::vector<int> sizes() const
    {
        static std::vector<int> sizes;
        if (sizes.empty()) {
            sizes.push_back(4);
            sizes.push_back(8);
            sizes.push_back(16);
            sizes.push_back(32);
            sizes.push_back(64);
            sizes.push_back(128);
            sizes.push_back(256);
            sizes.push_back(512);
            sizes.push_back(1024);
            sizes.push_back(2048);
            sizes.push_back(4096);
        }
        return sizes;
    }

    virtual std::vector<bool> executions() const
    {
        static std::vector<bool> executions;
        if (executions.empty()) {
            executions.push_back(false);
            executions.push_back(true);
        }
        return executions;
    }

    virtual double scaleFactor() const { return 1.0; }

    // OpenCV rounds integer division, Likely floors it.
    virtual bool ignoreOffByOne() const { return false; }

private:
    struct Speed
    {
        int iterations;
        double Hz;
        Speed() : iterations(-1), Hz(-1) {}
        Speed(int iter, clock_t startTime, clock_t endTime)
            : iterations(iter), Hz(double(iter) * CLOCKS_PER_SEC / (endTime-startTime)) {}
    };

    static likely_mat fromCvMat(const Mat &src)
    {
        likely_mat m = ::fromCvMat(src, true);
        if (!likely_floating(m->type) && (likely_depth(m->type) <= 16))
            likely_set_saturation(&m->type, true);
        return m;
    }

    void testCorrectness(likely_function_1 f, const cv::Mat &src) const
    {
        Mat dstOpenCV = computeBaseline(src);
        likely_mat srcLikely = fromCvMat(src);
        likely_mat dstLikely = f(srcLikely);

        Mat errorMat = abs(toCvMat(dstLikely) - dstOpenCV);
        errorMat.convertTo(errorMat, CV_32F);
        dstOpenCV.convertTo(dstOpenCV, CV_32F);
        errorMat = errorMat / (dstOpenCV + LIKELY_ERROR_TOLERANCE); // Normalize errors
        threshold(errorMat, errorMat, LIKELY_ERROR_TOLERANCE, 1, THRESH_BINARY);
        int errors = (int) norm(errorMat, NORM_L1);
        if (errors > 0) {
            likely_mat cvLikely = fromCvMat(dstOpenCV);
            stringstream errorLocations;
            errorLocations << "input\topencv\tlikely\trow\tcolumn\n";
            errors = 0;
            for (int i=0; i<src.rows; i++)
                for (int j=0; j<src.cols; j++)
                    if (errorMat.at<float>(i, j) == 1) {
                        const double src = likely_element(srcLikely, 0, j, i);
                        const double cv  = likely_element(cvLikely,  0, j, i);
                        const double dst = likely_element(dstLikely, 0, j, i);
                        if (ignoreOffByOne() && (cv-dst == 1)) continue;
                        if (errors < 100) errorLocations << src << "\t" << cv << "\t" << dst << "\t" << i << "\t" << j << "\n";
                        errors++;
                    }
            if (errors > 0)
                fprintf(stderr, "Test for %s differs in %d locations:\n%s", function(), errors, errorLocations.str().c_str());
            likely_release(cvLikely);
        }

        likely_release(srcLikely);
        likely_release(dstLikely);
    }

    Speed testBaselineSpeed(const cv::Mat &src) const
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

    Speed testLikelySpeed(likely_function_1 f, const cv::Mat &src) const
    {
        likely_mat srcLikely = fromCvMat(src);
        clock_t startTime, endTime;
        int iter = 0;
        startTime = endTime = clock();
        while ((endTime-startTime) / CLOCKS_PER_SEC < LIKELY_TEST_SECONDS) {
            likely_mat dstLikely = f(srcLikely);
            likely_release(dstLikely);
            endTime = clock();
            iter++;
        }
        likely_release(srcLikely);
        return Test::Speed(iter, startTime, endTime);
    }
};

class FloatingTest : public Test
{
    Mat computeBaseline(const Mat &src) const
    {
        if ((src.depth() == CV_32F) || (src.depth() == CV_64F))
            return computeFloatingBaseline(src);
        Mat floating;
        src.convertTo(floating, CV_32F);
        return computeFloatingBaseline(floating);
    }

    virtual Mat computeFloatingBaseline(const Mat &src) const = 0;
};

class ScalarFloatingTest : public FloatingTest
{
    Mat computeFloatingBaseline(const Mat &src) const
    {
        Mat dst(src.rows, src.cols, src.depth());
        const int elements = src.rows * src.cols;
        if (src.depth() == CV_32F) compute32f(reinterpret_cast<const float*>(src.data), reinterpret_cast<float*>(dst.data), elements);
        else                       compute64f(reinterpret_cast<const double*>(src.data), reinterpret_cast<double*>(dst.data), elements);
        return dst;
    }

    virtual void compute32f(const float *src, float *dst, int n) const = 0;
    virtual void compute64f(const double *src, double *dst, int n) const = 0;
};

class addTest : public Test {
    const char *function() const { return "add(32)"; }
    Mat computeBaseline(const Mat &src) const { Mat dst; add(src, 32, dst); return dst; }
};

class subtractTest : public Test {
    const char *function() const { return "subtract(32)"; }
    Mat computeBaseline(const Mat &src) const { Mat dst; subtract(src, 32, dst); return dst; }
};

class multiplyTest : public Test {
    const char *function() const { return "multiply(2)"; }
    Mat computeBaseline(const Mat &src) const { Mat dst; multiply(src, 2, dst); return dst; }
};

class divideTest : public Test {
    const char *function() const { return "divide(2)"; }
    Mat computeBaseline(const Mat &src) const { Mat dst; divide(src, 2, dst); return dst; }
    bool ignoreOffByOne() const { return true; }
};

class sqrtTest : public FloatingTest {
    const char *function() const { return "sqrt()"; }
    Mat computeFloatingBaseline(const Mat &src) const { Mat dst; sqrt(src, dst); return dst; }
};

class powiTest : public FloatingTest {
    const char *function() const { return "powi(3)"; }
    Mat computeFloatingBaseline(const Mat &src) const { Mat dst; pow(src, 3, dst); return dst; }
};

class sinTest : public ScalarFloatingTest {
    const char *function() const { return "sin()"; }
    void compute32f(const float *src, float *dst, int n) const { for (int i=0; i<n; i++) dst[i] = sinf(src[i]); }
    void compute64f(const double *src, double *dst, int n) const { for (int i=0; i<n; i++) dst[i] = sin(src[i]); }
};

class cosTest : public ScalarFloatingTest {
    const char *function() const { return "cos()"; }
    void compute32f(const float *src, float *dst, int n) const { for (int i=0; i<n; i++) dst[i] = cosf(src[i]); }
    void compute64f(const double *src, double *dst, int n) const { for (int i=0; i<n; i++) dst[i] = cos(src[i]); }
};

class powTest : public FloatingTest {
    const char *function() const { return "pow(1.5)"; }
    Mat computeFloatingBaseline(const Mat &src) const { Mat dst; pow(src, 1.5, dst); return dst; }
};

class expTest : public FloatingTest {
    const char *function() const { return "exp()"; }
    Mat computeFloatingBaseline(const Mat &src) const { Mat dst; exp(src, dst); return dst; }
    double scaleFactor() const { return 0.1; }
};

class exp2Test : public ScalarFloatingTest {
    const char *function() const { return "exp2()"; }
    void compute32f(const float *src, float *dst, int n) const { for (int i=0; i<n; i++) dst[i] = exp2f(src[i]); }
    void compute64f(const double *src, double *dst, int n) const { for (int i=0; i<n; i++) dst[i] = exp2(src[i]); }
    double scaleFactor() const { return 0.1; }
};

class logTest : public FloatingTest {
    const char *function() const { return "log()"; }
    Mat computeFloatingBaseline(const Mat &src) const { Mat dst; log(src, dst); return dst; }
};

class log10Test : public ScalarFloatingTest {
    const char *function() const { return "log10()"; }
    void compute32f(const float *src, float *dst, int n) const { for (int i=0; i<n; i++) dst[i] = log10f(src[i]); }
    void compute64f(const double *src, double *dst, int n) const { for (int i=0; i<n; i++) dst[i] = log10(src[i]); }
};

class log2Test : public ScalarFloatingTest {
    const char *function() const { return "log2()"; }
    void compute32f(const float *src, float *dst, int n) const { for (int i=0; i<n; i++) dst[i] = log2f(src[i]); }
    void compute64f(const double *src, double *dst, int n) const { for (int i=0; i<n; i++) dst[i] = log2(src[i]); }
};

class maddTest : public Test {
    const char *function() const { return "madd(2,3)"; }
    Mat computeBaseline(const Mat &src) const { Mat dst; src.convertTo(dst, src.depth(), 2, 3); return dst; }
};

int main(int argc, char *argv[])
{
    // Parse arguments
    for (int i=1; i<argc; i++) {
        if      (!strcmp("-h", argv[i])) printf("benchmark [--examples] [-function <str>] [-type <type>] [-size <int>]\n");
        else if (!strcmp("--examples", argv[i])) BenchmarkExamples = true;
        else if (!strcmp("--serial", argv[i])) BenchmarkExecution = false;
        else if (!strcmp("--parallel", argv[i])) BenchmarkExecution = true;
        else if (!strcmp("-function", argv[i])) BenchmarkFunction = argv[++i];
        else if (!strcmp("-type", argv[i])) BenchmarkType = likely_type_from_string(argv[++i]);
        else if (!strcmp("-size", argv[i])) BenchmarkSize = atoi(argv[++i]);
        else    { printf("Unrecognized argument: %s\nTry running 'benchmark -h' for help", argv[i]); return 1; }
    }

    // Print to console immediately
    setbuf(stdout, NULL);

    if (BenchmarkExamples) {
        printf("Example \tSpeed\n");
        lua_State *L = likely_exec("");
        lua_getfield(L, -1, "likely");
        lua_getfield(L, -1, "examples");
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            Test::runExample(lua_tostring(L, -1));
            lua_pop(L, 1);
        }
        lua_pop(L, 2);
        lua_close(L);
    } else {
        printf("Function \tType \tSize \tExecution \tSpeedup\n");
        addTest().run();
        subtractTest().run();
        multiplyTest().run();
        divideTest().run();
        sqrtTest().run();
        powiTest().run();
        sinTest().run();
        cosTest().run();
        powTest().run();
        expTest().run();
        exp2Test().run();
        logTest().run();
        log10Test().run();
        log2Test().run();
//        maddTest().run();
//        logTest().run();
    }

    return 0;
}

