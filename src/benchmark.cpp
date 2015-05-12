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

#include <ctime>
#include <fstream>
#include <iostream>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Path.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <likely.h>
#include <likely/opencv.hpp>

using namespace cv;
using namespace llvm;
using namespace std;

const double ErrorTolerance = 0.000001;
const int TestSeconds = 1;

static cl::opt<bool> BenchmarkTest("test", cl::desc("Run tests for correctness only"));
static cl::opt<bool> BenchmarkMulticore("multi-core", cl::desc("Compile multi-core kernels"));
static cl::alias     BenchmarkMulticoreA("m", cl::desc("Alias for -multi-core"), cl::aliasopt(BenchmarkMulticore));
static cl::opt<bool> BenchmarkQuiet("quiet", cl::desc("Don't print benchmark output"));
static cl::alias     BenchmarkQuietA("q", cl::desc("Alias for -quiet"), cl::aliasopt(BenchmarkQuiet));
static cl::opt<bool> BenchmarkVerbose("verbose", cl::desc("Verbose compiler output"));
static cl::alias     BenchmarkVerboseA("v", cl::desc("Alias for -verbose"), cl::aliasopt(BenchmarkVerbose));
static cl::opt<bool> BenchmarkHuman("human", cl::desc("Optimize compiler output for human readability"));
static cl::alias     BenchmarkHumanA("h", cl::desc("Alias for -human"), cl::aliasopt(BenchmarkHuman));
static cl::opt<string> BenchmarkFile("file", cl::desc("Benchmark the specified file only"), cl::value_desc("filename"));
static cl::opt<string> BenchmarkFunction("function", cl::desc("Benchmark the specified function only"), cl::value_desc("string"));
static cl::opt<string> BenchmarkType("type", cl::desc("Benchmark the specified type only"), cl::value_desc("type"));
static cl::opt<string> BenchmarkRoot("root", cl::desc("Root of the Likely repository"), cl::value_desc("path"));
static cl::opt<int>    BenchmarkSize("size", cl::desc("Benchmark the specified size only"));

static void checkRead(const void *data, const char *fileName)
{
    likely_ensure(data != NULL, "failed to read \"%s\", did you forget to set '-root' or run 'benchmark' from the root of the Likely repository?", fileName);
}

static string resolvePath(const string &fileName)
{
    if (BenchmarkRoot.empty())
        return fileName;
    SmallString<16> resolvedPath;
    sys::path::append(resolvedPath, BenchmarkRoot, fileName);
    return resolvedPath.str();
}

static vector<Mat> generateData(int rows, int columns, likely_type type, bool color, bool imageSet)
{
    vector<Mat> original;
    if (imageSet) {

    } else {
        const char *const lenna = "data/misc/lenna.tiff";
        const Mat m = imread(resolvePath(lenna));
        checkRead(m.data, lenna);
        original.push_back(m);
    }

    vector<Mat> cvt;
    if (color) {
        cvt = original;
    } else {
        for (const Mat &m : original) {
            Mat n;
            cvtColor(m, n, CV_BGR2GRAY);
            cvt.push_back(n);
        }
    }

    vector<Mat> resized;
    for (const Mat &m : cvt) {
        Mat n;
        resize(m, n, Size(columns, rows), 0, 0, INTER_NEAREST);
        n.convertTo(n, likelyToOpenCVDepth(type));
        resized.push_back(n);
    }

    return resized;
}

struct TestBase
{
    void run(likely_const_env parent) const
    {
        if (!BenchmarkFunction.empty() && (name() != BenchmarkFunction))
            return;

        stringstream source;
        stringstream fileName;
        fileName << "library/" << name() << ".md";
        const likely_const_mat fileSource = likely_read(resolvePath(fileName.str()).c_str(), likely_file_gfm, likely_text);
        checkRead(fileSource, fileName.str().c_str());
        source << fileSource->data;
        likely_release_mat(fileSource);

        source << "    (extern multi-dimension \"" << name() << "\" (";
        for (int i=0; i<additionalParameters(); i++)
            source << " multi-dimension";
        source << ") " << name() << " true)";
        const likely_const_env env = likely_lex_parse_and_eval(source.str().c_str(), likely_file_gfm, parent);
        void *const f = likely_function(env->expr);
        assert(f);

        for (const likely_type type : types()) {
            if (!BenchmarkType.empty() && (type != likely_type_from_string(BenchmarkType.c_str(), NULL)))
                continue;

            for (const int size : sizes()) {
                if (BenchmarkTest && (size != 128)) continue;
                if (BenchmarkSize && (size != BenchmarkSize)) continue;

                // Generate input matrix
                const vector<Mat> srcCV = generateData(size, size, type, color(), imageSet());
                const likely_mat likelySrc = likelyFromOpenCVMats(srcCV);
                if (!(likelySrc->type & likely_floating) && ((likelySrc->type & likely_depth) <= 16))
                    likelySrc->type |= likely_saturated; // Follow OpenCV's saturation convention
                vector<likely_const_mat> likelyArgs = additionalArguments(likelySrc);
                likelyArgs.insert(likelyArgs.begin(), likelySrc);

                const likely_const_mat typeString = likely_type_to_string(type);
                if (!BenchmarkQuiet)
                    printf("%s \t%s \t%d \t%s\t", name(), typeString->data, size, BenchmarkMulticore ? "m" : "s");
                likely_release_mat(typeString);
                testCorrectness(reinterpret_cast<likely_mat (*)(const likely_const_mat*)>(f), srcCV, likelyArgs.data());

                if (!BenchmarkTest) {
                    const Speed baseline = testBaselineSpeed(srcCV);
                    const Speed likely = testLikelySpeed(reinterpret_cast<likely_mat (*)(const likely_const_mat*)>(f), likelyArgs.data());
                    if (!BenchmarkQuiet)
                        printf("%-8.3g \t%.3gx\n", double(likely.iterations), likely.Hz/baseline.Hz);
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

    static void runFile(const char *fileName, likely_const_env const parent)
    {
        if (!BenchmarkFile.empty() && (fileName != BenchmarkFile))
            return;

        const likely_const_mat source = likely_read(resolvePath(string("library/") + string(fileName) + string(".md")).c_str(), likely_file_gfm, likely_text);
        checkRead(source, fileName);

        if (!BenchmarkQuiet)
            printf("%s \t%s \t", fileName, BenchmarkMulticore ? "m" : "s");
        likely_release_env(likely_lex_parse_and_eval(source->data, likely_file_gfm, parent));

        if (BenchmarkTest) {
            if (!BenchmarkQuiet)
                printf("\n");
        } else {
            clock_t startTime, endTime;
            int iter = 0;
            startTime = endTime = clock();
            while ((endTime-startTime) / CLOCKS_PER_SEC < TestSeconds) {
                likely_release_env(likely_lex_parse_and_eval(source->data, likely_file_gfm, parent));
                endTime = clock();
                iter++;
            }
            Speed speed(iter, startTime, endTime);
            if (!BenchmarkQuiet)
                printf("%.2e\n", speed.Hz);
        }

        likely_release_mat(source);
    }

protected:
    virtual const char *name() const = 0;
    virtual Mat computeBaseline(const vector<Mat> &src) const = 0;
    virtual int additionalParameters() const = 0;
    virtual bool color() const = 0;
    virtual bool imageSet() const = 0;

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

    virtual vector<int> sizes() const
    {
        vector<int> sizes;
        sizes.push_back(8);
        sizes.push_back(32);
        sizes.push_back(128);
        sizes.push_back(512);
        sizes.push_back(2048);
        return sizes;
    }

private:
    struct Speed
    {
        int iterations;
        double Hz;
        Speed() : iterations(-1), Hz(-1) {}
        Speed(int iter, clock_t startTime, clock_t endTime)
            : iterations(iter), Hz(double(iter) * CLOCKS_PER_SEC / (endTime-startTime)) {}
    };

    void testCorrectness(likely_mat (*f)(const likely_const_mat*), const vector<Mat> &srcCV, const likely_const_mat *srcLikely) const
    {
        Mat dstOpenCV = computeBaseline(srcCV);
        const likely_const_mat dstLikely = f(srcLikely);

        Mat errorMat = abs(likelyToOpenCVMat(dstLikely) - dstOpenCV);
        errorMat.convertTo(errorMat, CV_32F);
        dstOpenCV.convertTo(dstOpenCV, CV_32F);
        errorMat /= (dstOpenCV + ErrorTolerance); // Normalize errors
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
                            if (errors < 100) errorLocations << cv << "\t" << dst << "\t" << i << "\t" << j << "\t" << k << "\n";
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

    Speed testBaselineSpeed(const vector<Mat> &src) const
    {
        clock_t startTime, endTime;
        int iter = 0;
        startTime = endTime = clock();
        while ((endTime-startTime) / CLOCKS_PER_SEC < TestSeconds) {
            computeBaseline(src);
            endTime = clock();
            iter++;
        }
        return TestBase::Speed(iter, startTime, endTime);
    }

    Speed testLikelySpeed(likely_mat (*f)(const likely_const_mat*), const likely_const_mat *srcLikely) const
    {
        clock_t startTime, endTime;
        int iter = 0;
        startTime = endTime = clock();
        while ((endTime-startTime) / CLOCKS_PER_SEC < TestSeconds) {
            likely_const_mat dstLikely = f(srcLikely);
            likely_release_mat(dstLikely);
            endTime = clock();
            iter++;
        }
        return TestBase::Speed(iter, startTime, endTime);
    }
};

template <int const N = 0, bool const C = true, bool const IS = false>
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

    bool imageSet() const
    {
        return IS;
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

    Mat computeBaseline(const vector<Mat> &src) const
    {
        Mat dst;
        threshold(src[0], dst, 127, 1, THRESH_BINARY);
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

    Mat computeBaseline(const vector<Mat> &src) const
    {
        Mat dst;
        src[0].convertTo(dst, src[0].depth(), 3.141592, 2.718281);
        return dst;
    }
};

class MinMaxLoc : public Test<0, false>
{
    const char *name() const
    {
        return "min-max-loc";
    }

    Mat computeBaseline(const vector<Mat> &src) const
    {
        double minVal, maxVal;
        Point minLoc, maxLoc;
        minMaxLoc(src[0], &minVal, &maxVal, &minLoc, &maxLoc);

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

    Mat computeBaseline(const vector<Mat> &src) const
    {
        Mat dst;
        cvtColor(src[0], dst, CV_BGR2GRAY);
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

    Mat computeBaseline(const vector<Mat> &src) const
    {
        Mat dst;
        normalize(src[0], dst);
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

class MatrixMultiplication : public Test<1, false>
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

    Mat computeBaseline(const vector<Mat> &src) const
    {
        Mat dst = src[0] * src[0];
        return dst;
    }

    vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_f32);
        types.push_back(likely_f64);
        return types;
    }

    vector<int> sizes() const
    {
        vector<int> sizes;
        sizes.push_back(8);
        sizes.push_back(32);
        sizes.push_back(128);
        sizes.push_back(512);
        return sizes;
    }
};

class GEMM : public Test<4, false>
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

    Mat computeBaseline(const vector<Mat> &src) const
    {
        Mat dst;
        gemm(src[0], src[0], 3.141592, src[0], 2.718281, dst);
        return dst;
    }

    vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_f32);
        types.push_back(likely_f64);
        return types;
    }

    vector<int> sizes() const
    {
        vector<int> sizes;
        sizes.push_back(8);
        sizes.push_back(32);
        sizes.push_back(128);
        sizes.push_back(512);
        return sizes;
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

    Mat computeBaseline(const vector<Mat> &src) const
    {
        Mat dst;
        matchTemplate(src[0], templ, dst, CV_TM_CCORR);
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
        : templ(generateData(8, 8, likely_f32, false, false)[0]) {}
};

class MeanCenter : public Test<0, false, true>
{
    const char *name() const
    {
        return "mean-center";
    }

    Mat computeBaseline(const vector<Mat> &) const
    {
        Mat dst;
        return dst;
    }

    vector<likely_type> types() const
    {
        vector<likely_type> types;
        types.push_back(likely_f32);
        return types;
    }
};

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    // Print to the console immediately
    setbuf(stdout, NULL);

    likely_settings settings = likely_default_settings(BenchmarkHuman ? likely_file_ir : likely_file_object, false);
    settings.multicore = BenchmarkMulticore;
    settings.verbose = BenchmarkVerbose;
    const likely_const_env parent = likely_standard(settings, NULL, likely_file_void);

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
