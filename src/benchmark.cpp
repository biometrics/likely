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

#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <fstream>
#include <iostream>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Path.h>
#include <opencv2/imgproc.hpp>
#include <likely.h>
#include <likely/opencv.hpp>
#include <likely/timer.hpp>

using namespace cv;
using namespace llvm;
using namespace std;

const double ErrorTolerance = 0.00001;

static cl::opt<bool> BenchmarkTest("test", cl::desc("Run tests for correctness only"));
static cl::opt<bool> BenchmarkMulticore("multi-core", cl::desc("Compile multi-core kernels"));
static cl::alias     BenchmarkMulticoreA("m", cl::desc("Alias for -multi-core"), cl::aliasopt(BenchmarkMulticore));
static cl::opt<bool> BenchmarkQuiet("quiet", cl::desc("Don't print benchmark output"));
static cl::alias     BenchmarkQuietA("q", cl::desc("Alias for -quiet"), cl::aliasopt(BenchmarkQuiet));
static cl::opt<bool> BenchmarkVerbose("verbose", cl::desc("Verbose compiler output"));
static cl::alias     BenchmarkVerboseA("v", cl::desc("Alias for -verbose"), cl::aliasopt(BenchmarkVerbose));
static cl::opt<int> BenchmarkOptimizationLevel("optimization-level", cl::desc("Compiler optimization level (0-2)"), cl::init(2));
static cl::alias    BenchmarkOptimizationLevelA("o", cl::desc("Alias for -optimization-level"), cl::aliasopt(BenchmarkOptimizationLevel));
static cl::opt<string> BenchmarkFile("file", cl::desc("Benchmark the specified file only"), cl::value_desc("filename"));
static cl::opt<string> BenchmarkFunction("function", cl::desc("Benchmark the specified function only"), cl::value_desc("string"));
static cl::opt<string> BenchmarkType("type", cl::desc("Benchmark the specified type only"), cl::value_desc("type"));
static cl::opt<string> BenchmarkRoot("root", cl::desc("Root of the Likely repository"), cl::value_desc("path"));
static cl::opt<int>    BenchmarkSize("size", cl::desc("Benchmark the specified size only"));

static string resolvePath(const string &fileName)
{
    if (BenchmarkRoot.empty())
        return fileName;
    SmallString<16> resolvedPath;
    sys::path::append(resolvedPath, BenchmarkRoot, fileName);
    return resolvedPath.str();
}

static Mat generateData(int rows, int columns, likely_type type, bool color)
{
    static Mat preread;
    if (!preread.data) {
        const likely_const_mat m = likely_read(resolvePath("data/misc/lenna.tiff").data(),
                                               likely_file_media,
                                               likely_image);
        preread = likelyToOpenCVMat(m);
        likely_release_mat(m);
    }

    Mat original;
    if (color) original = preread;
    else       cvtColor(preread, original, CV_BGR2GRAY);

    Mat scaled;
    resize(original, scaled, Size(columns, rows), 0, 0, INTER_NEAREST);
    scaled.convertTo(scaled, likelyToOpenCVDepth(type));
    return scaled;
}

struct TestBase
{
    void run(likely_const_env parent) const
    {
        if (!BenchmarkFunction.empty() && (name() != BenchmarkFunction))
            return;

        stringstream fileName;
        fileName << "library/" << name() << ".md";
        likely_const_env env = likely_retain_env(parent);
        likely_read_lex_parse_and_eval(resolvePath(fileName.str()).c_str(), &env);

        stringstream source;
        source << "    (extern multi-dimension \"" << name() << "\" (";
        for (int i=0; i<additionalParameters(); i++)
            source << " multi-dimension";
        source << ") " << name() << " ())";
        likely_lex_parse_and_eval(source.str().c_str(), likely_file_gfm, &env);
        void *const f = likely_function(env->expr);
        assert(f);

        for (const likely_type type : types()) {
            if (!BenchmarkType.empty() && (type != likely_type_from_string(BenchmarkType.c_str(), NULL)))
                continue;

            for (const int size : sizes()) {
                if (BenchmarkTest && (size != 128)) continue;
                if (BenchmarkSize && (size != BenchmarkSize)) continue;

                // Generate input matrix
                const Mat srcCV = preprocess(generateData(size, size, type, color()));

                const likely_mat likelySrc = likelyFromOpenCVMat(srcCV, true); // A convenient opportunity to test `likely_indirect`
                if (!(likelySrc->type & likely_floating) && ((likelySrc->type & likely_depth) <= 16))
                    likelySrc->type |= likely_saturated; // Follow OpenCV's saturation convention
                vector<likely_const_mat> likelyArgs = additionalArguments(likelySrc);
                likelyArgs.insert(likelyArgs.begin(), likelySrc);

                if (!BenchmarkQuiet) {
                    const likely_const_mat typeString = likely_type_to_string(type);
                    printf("%s \t%s \t%d \t%s\t", name(), typeString->data, size, BenchmarkMulticore ? "m" : "s");
                    likely_release_mat(typeString);
                }

                testCorrectness(reinterpret_cast<likely_mat (*)(const likely_const_mat*)>(f), srcCV, likelyArgs.data());

                if (!BenchmarkTest) {
                    const LikelyTimer baseline = LikelyTimer::measure([&]()->void { computeBaseline(srcCV); });
                    const LikelyTimer likely = LikelyTimer::measure([&]()->void { likely_release_mat(reinterpret_cast<likely_mat (*)(const likely_const_mat*)>(f)(likelyArgs.data())); });
                    if (!BenchmarkQuiet)
                        printf("%-8.3g \t%.3gx\n", double(likely.numIterations), LikelyTimer::speedup(baseline,likely));
                } else {
                    if (!BenchmarkQuiet)
                        printf("\n");
                }

                for (const likely_const_mat &likelyArg : likelyArgs)
                    likely_release_mat(likelyArg);
            }
        }

        likely_release_env(env);
    }

    static void runFile(const char *fileName, likely_const_env parent)
    {
        if (!BenchmarkFile.empty() && (fileName != BenchmarkFile))
            return;

        const string resolvedFileName = resolvePath(string("library/") + string(fileName) + string(".md")).c_str();

        if (!BenchmarkQuiet)
            printf("%s \t%s \t", fileName, BenchmarkMulticore ? "m" : "s");

        {
            likely_const_env env = likely_retain_env(parent);
            likely_read_lex_parse_and_eval(resolvedFileName.c_str(), &env);
            likely_release_env(env);
        }

        if (BenchmarkTest) {
            if (!BenchmarkQuiet)
                printf("\n");
        } else {
            const LikelyTimer speed = LikelyTimer::measure([&]()->void { likely_const_env env = likely_retain_env(parent);
                                                                         likely_read_lex_parse_and_eval(resolvedFileName.c_str(), &env);
                                                                         likely_release_env(env); });
            if (!BenchmarkQuiet)
                printf("%.2e\n", speed.Hz);
        }
    }

protected:
    virtual const char *name() const = 0;
    virtual Mat preprocess(const Mat &src) const { return src; }
    virtual Mat computeBaseline(const Mat &src) const = 0;
    virtual int additionalParameters() const = 0;
    virtual bool color() const = 0;
    virtual int maxSize() const = 0;

    virtual vector<likely_const_mat> additionalArguments(likely_const_mat) const
    {
        return vector<likely_const_mat>();
    }

    virtual vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_u8);
        types.push_back(likely_i16);
        types.push_back(likely_i32);
        types.push_back(likely_f32);
        types.push_back(likely_f64);
        return types;
    }

    vector<int> sizes() const
    {
        vector<int> sizes;
        if (maxSize() >= 8)    sizes.push_back(8);
        if (maxSize() >= 32)   sizes.push_back(32);
        if (maxSize() >= 128)  sizes.push_back(128);
        if (maxSize() >= 512)  sizes.push_back(512);
        if (maxSize() >= 2048) sizes.push_back(2048);
        return sizes;
    }

private:
    void checkAxis(uint32_t expected, uint32_t actual) const
    {
        if (expected == actual)
            return;

        fprintf(stderr, "Test for: %s differs in channel count, exptected: %d but got %d", name(), expected, actual);
        exit(EXIT_FAILURE);
    }

    void testCorrectness(likely_mat (*f)(const likely_const_mat*), const Mat &srcCV, const likely_const_mat *srcLikely) const
    {
        Mat dstOpenCV = computeBaseline(srcCV);
        const likely_const_mat dstLikely = f(srcLikely);

        if (!dstLikely) {
            fprintf(stderr, "Likely returned NULL!");
            exit(EXIT_FAILURE);
        }

        checkAxis(dstOpenCV.channels(), dstLikely->channels);
        checkAxis(dstOpenCV.cols      , dstLikely->columns );
        checkAxis(dstOpenCV.rows      , dstLikely->rows    );
        checkAxis(1                   , dstLikely->frames  );

        Mat errorMat;
        absdiff(likelyToOpenCVMat(dstLikely), dstOpenCV, errorMat);
        errorMat.convertTo(errorMat, CV_32F);
        dstOpenCV.convertTo(dstOpenCV, CV_32F);
        errorMat /= (abs(dstOpenCV) + 1); // Normalize errors
        threshold(errorMat, errorMat, ErrorTolerance, 1, THRESH_BINARY);

        if (norm(errorMat, NORM_L1) > 0) {
            const likely_const_mat cvLikely = likelyFromOpenCVMat(dstOpenCV);
            stringstream errorLocations;
            errorLocations << "opencv\tlikely\trow\tcolumn\tchannel\n";
            int errors = 0;
            const int channels = dstOpenCV.channels();
            for (int i=0; i<dstOpenCV.rows; i++)
                for (int j=0; j<dstOpenCV.cols; j++)
                    for (int k=0; k<channels; k++)
                        if (errorMat.at<float>(i, j*channels + k) == 1) {
                            const double cv  = likely_get_element(cvLikely , k, j, i, 0);
                            const double dst = likely_get_element(dstLikely, k, j, i, 0);
                            if (errors < 100) errorLocations << cv << " \t" << dst << " \t" << i << " \t" << j << " \t" << k << "\n";
                            errors++;
                        }
            if (errors > 0) {
                fprintf(stderr, "Test for: %s differs in: %d location(s):\n%s", name(), errors, errorLocations.str().c_str());
                exit(EXIT_FAILURE);
            }
            likely_release_mat(cvLikely);
        }

        likely_release_mat(dstLikely);
    }
};

template <int const N = 0, bool const C = true, const int MS = 2048>
struct Test : public TestBase
{
    int additionalParameters() const
    {
        return N;
    }

    bool color() const
    {
        return C;
    }

    int maxSize() const
    {
        return MS;
    }
};

class BinaryThreshold : public Test<2>
{
    const char *name() const
    {
        return "binary-threshold";
    }

    vector<likely_const_mat> additionalArguments(likely_const_mat src) const
    {
        const likely_type scalarType = src->type & likely_element & ~likely_saturated;
        vector<likely_const_mat> args;
        const double thresh = 127;
        const double maxval = 1;
        args.push_back(likely_scalar(scalarType, &thresh, 1));
        args.push_back(likely_scalar(scalarType, &maxval, 1));
        return args;
    }

    Mat computeBaseline(const Mat &src) const
    {
        Mat dst;
        threshold(src, dst, 127, 1, THRESH_BINARY);
        return dst;
    }

    vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_u8);
        types.push_back(likely_i16);
        types.push_back(likely_f32);
        return types;
    }
};

class MultiplyAdd : public Test<2>
{
    const char *name() const
    {
        return "multiply-add";
    }

    vector<likely_const_mat> additionalArguments(likely_const_mat src) const
    {
        const likely_type scalarType = ((src->type & likely_depth) <= 32) ? likely_f32 : likely_f64;
        vector<likely_const_mat> args;
        const double alpha = 3.141592;
        const double beta = 2.718281;
        args.push_back(likely_scalar(scalarType, &alpha, 1));
        args.push_back(likely_scalar(scalarType, &beta, 1));
        return args;
    }

    Mat computeBaseline(const Mat &src) const
    {
        Mat dst;
        src.convertTo(dst, src.depth(), 3.141592, 2.718281);
        return dst;
    }
};

class MinMaxLoc : public Test<0, false>
{
    const char *name() const
    {
        return "min-max-loc";
    }

    Mat computeBaseline(const Mat &src) const
    {
        double minVal, maxVal;
        Point minLoc, maxLoc;
        minMaxLoc(src, &minVal, &maxVal, &minLoc, &maxLoc);

        Mat result(2, 3, CV_64FC1);
        result.at<double>(0, 0) = minVal;
        result.at<double>(0, 1) = minLoc.x;
        result.at<double>(0, 2) = minLoc.y;
        result.at<double>(1, 0) = maxVal;
        result.at<double>(1, 1) = maxLoc.x;
        result.at<double>(1, 2) = maxLoc.y;
        return result;
    }
};

class ConvertGrayscale : public Test<>
{
    const char *name() const
    {
        return "convert-grayscale";
    }

    Mat computeBaseline(const Mat &src) const
    {
        Mat dst;
        cvtColor(src, dst, CV_BGR2GRAY);
        return dst;
    }

    vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_u8);
        types.push_back(likely_u16);
        types.push_back(likely_f32);
        return types;
    }
};

class NormalizeL2 : public Test<>
{
    const char *name() const
    {
        return "normalize-l2";
    }

    Mat computeBaseline(const Mat &src) const
    {
        Mat dst;
        normalize(src, dst);
        return dst;
    }

    vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_f32);
        types.push_back(likely_f64);
        return types;
    }
};

class MatrixMultiplication : public Test<1, false, 512>
{
    const char *name() const
    {
        return "matrix-multiplication";
    }

    vector<likely_const_mat> additionalArguments(likely_const_mat src) const
    {
        vector<likely_const_mat> args;
        args.push_back(likely_retain_mat(src));
        return args;
    }

    Mat computeBaseline(const Mat &src) const
    {
        Mat dst = src * src;
        return dst;
    }

    vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_f32);
        types.push_back(likely_f64);
        return types;
    }
};

class GEMM : public Test<4, false, 512>
{
    const char *name() const
    {
        return "gemm";
    }

    vector<likely_const_mat> additionalArguments(likely_const_mat src) const
    {
        vector<likely_const_mat> args;
        const double alpha = 3.141592;
        const double beta = 2.718281;
        args.push_back(likely_retain_mat(src));
        args.push_back(likely_scalar(likely_double, &alpha, 1));
        args.push_back(likely_retain_mat(src));
        args.push_back(likely_scalar(likely_double, &beta, 1));
        return args;
    }

    Mat computeBaseline(const Mat &src) const
    {
        Mat dst;
        gemm(src, src, 3.141592, src, 2.718281, dst);
        return dst;
    }

    vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_f32);
        types.push_back(likely_f64);
        return types;
    }
};

class MatchTemplate : public Test<1, false>
{
    const Mat templ;

    const char *name() const
    {
        return "match-template";
    }

    vector<likely_const_mat> additionalArguments(likely_const_mat) const
    {
        vector<likely_const_mat> args;
        args.push_back(likelyFromOpenCVMat(templ));
        return args;
    }

    Mat computeBaseline(const Mat &src) const
    {
        Mat dst;
        matchTemplate(src, templ, dst, CV_TM_CCORR);
        return dst;
    }

    vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_f32);
        return types;
    }

public:
    MatchTemplate()
        : templ(generateData(8, 8, likely_f32, false)) {}
};

class Filter2D : public Test<1, false>
{
    const Mat kernel;

    const char *name() const
    {
        return "filter-2D";
    }

    vector<likely_const_mat> additionalArguments(likely_const_mat) const
    {
        vector<likely_const_mat> args;
        args.push_back(likelyFromOpenCVMat(kernel));
        return args;
    }

    Mat computeBaseline(const Mat &src) const
    {
        Mat dst;
        filter2D(src, dst, src.depth() == CV_64F ? CV_64F : CV_32F, kernel, Point(-1, -1), 0, BORDER_CONSTANT);
        return dst;
    }

    vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_u8);
        types.push_back(likely_i16);
        types.push_back(likely_f32);
        types.push_back(likely_f64);
        return types;
    }

 public:
    Filter2D()
        : kernel(generateData(3, 3, likely_f32, false)) {}
 };

class Average : public Test<0, false>
{
    const char *name() const
    {
        return "average";
    }

    Mat computeBaseline(const Mat &src) const
    {
        Mat average;
        reduce(src, average, 0, CV_REDUCE_AVG, src.depth() == CV_64F ? CV_64F : CV_32F);
        return average;
    }

    vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_u8);
        types.push_back(likely_i16);
        types.push_back(likely_f32);
        types.push_back(likely_f64);
        return types;
    }
};

class Transpose : public Test<0, false>
{
    const char *name() const
    {
        return "transpose";
    }

    Mat computeBaseline(const Mat &src) const
    {
        Mat dst;
        transpose(src, dst);
        return dst;
    }
};

class MultiplyTransposed : public Test<1, false, 512>
{
    const char *name() const
    {
        return "multiply-transposed";
    }

    mutable Mat delta;
    vector<likely_const_mat> additionalArguments(likely_const_mat src) const
    {
        vector<likely_const_mat> args;
        delta = Mat::ones(1, src->columns, ((src->type & likely_depth) == 64) ? CV_64F : CV_32F);
        args.push_back(likelyFromOpenCVMat(delta));
        return args;
    }

    Mat computeBaseline(const Mat &src) const
    {
        Mat dst;
        mulTransposed(src, dst, true, delta);
        return dst;
    }

    vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_u8);
        types.push_back(likely_i16);
        types.push_back(likely_f32);
        types.push_back(likely_f64);
        return types;
    }
};

class Covariance : public Test<0, false, 512>
{
    const char *name() const
    {
        return "covariance";
    }

    Mat computeBaseline(const Mat &src) const
    {
        Mat cov, mean;
        calcCovarMatrix(src, cov, mean, CV_COVAR_NORMAL | CV_COVAR_ROWS, src.depth() == CV_64F ? CV_64F : CV_32F);
        return cov;
    }

    vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_u8);
        types.push_back(likely_i16);
        types.push_back(likely_f32);
        types.push_back(likely_f64);
        return types;
    }
};

class Sort : public Test<0, false, 512>
{
    const char *name() const
    {
        return "sort";
    }

    Mat computeBaseline(const Mat &src) const
    {
        Mat dst = src;
        cv::sort(src, dst, CV_SORT_EVERY_ROW);
        return dst;
    }
};

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    // Print to the console immediately
    setbuf(stdout, NULL);

    likely_settings settings = likely_default_settings(likely_file_void, BenchmarkVerbose);
    settings.optimization_level = BenchmarkOptimizationLevel;
    settings.multicore = BenchmarkMulticore;
    settings.heterogeneous = false;
    const likely_const_env parent = likely_standard(settings);

    if (!BenchmarkQuiet) {
        const time_t now = time(0);
        char dateTime[80];
        strftime(dateTime, sizeof(dateTime), "%Y-%m-%d.%X", localtime(&now));
        puts(dateTime);
        puts("");
        puts("To reproduce the following results, run the `benchmark` application included in a build of Likely.");
    }

    if (BenchmarkFile.empty()) {
        if (!BenchmarkQuiet) {
            puts("");
            puts("Likely vs. OpenCV Benchmark Results");
            puts("-----------------------------------");
            puts("Function: Benchmarked function name");
            puts("    Type: Matrix element data type");
            puts("    Size: Matrix rows and columns");
            puts("    Exec: (s)ingle-core or (m)ulti-core");
            puts("    Iter: Times Likely function was run in one second");
            puts(" Speedup: Execution speed of Likely relative to OpenCV");
            puts("");
            puts("Function \t\tType \tSize \tExec \tIter \t\tSpeedup");
        }

        BinaryThreshold().run(parent);
        ConvertGrayscale().run(parent);
        MultiplyAdd().run(parent);
        MinMaxLoc().run(parent);
        NormalizeL2().run(parent);
        MatrixMultiplication().run(parent);
        GEMM().run(parent);
        MatchTemplate().run(parent);
        Filter2D().run(parent);
        Average().run(parent);
        Transpose().run(parent);
        MultiplyTransposed().run(parent);
        Covariance().run(parent);
        Sort().run(parent);
    }

    if (BenchmarkFunction.empty()) {
        if (!BenchmarkQuiet) {
            puts("");
            puts("Likely Demo Benchmark Results");
            puts("-----------------------------");
            puts("File: Benchmarked file name");
            puts("Exec: (s)ingle-core or (m)ulti-core");
            puts("Iter: Times Likely file was run in one second");
            puts("");
            puts("File \t\tExec \tIter");
        }

        TestBase::runFile("gabor_wavelet", parent);
        TestBase::runFile("mandelbrot_set", parent);
    }

    likely_release_env(parent);

    likely_shutdown();
    return EXIT_SUCCESS;
}
