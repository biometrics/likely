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
static cl::opt<bool> BenchmarkVerbose("verbose", cl::desc("Verbose compiler output"));
static cl::alias     BenchmarkVerboseA("v", cl::desc("Alias for -verbose"), cl::aliasopt(BenchmarkVerbose));
static cl::opt<bool> BenchmarkHuman("human", cl::desc("Optimize compiler output for human readability"));
static cl::alias     BenchmarkHumanA("h", cl::desc("Alias for -human"), cl::aliasopt(BenchmarkHuman));
static cl::opt<string> BenchmarkFile("file", cl::desc("Benchmark the specified file only"), cl::value_desc("filename"));
static cl::opt<string> BenchmarkFunction("function", cl::desc("Benchmark the specified function only"), cl::value_desc("string"));
static cl::opt<string> BenchmarkType("type", cl::desc("Benchmark the specified type only"), cl::value_desc("type"));

static void checkRead(const void *data, const char *fileName)
{
    likely_ensure(data != NULL, "failed to read \"%s\", did you forget to run 'benchmark' from the root of the repository?", fileName);
}

static Mat generateData(int rows, int columns, likely_type type, bool color)
{
    static Mat original;
    if (!original.data) {
        original = imread("data/misc/lenna.tiff");
        checkRead(original.data, "data/misc/lenna.tiff");
    }

    Mat m;
    if (color) m = original;
    else       cvtColor(original, m, CV_BGR2GRAY);

    Mat n;
    resize(m, n, Size(columns, rows), 0, 0, INTER_NEAREST);
    n.convertTo(n, likelyToOpenCVDepth(type));
    return n;
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
        const likely_const_mat fileSource = likely_read(fileName.str().c_str(), likely_file_gfm, likely_text);
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

                // Generate input matrix
                const Mat srcCV = generateData(size, size, type, color());
                const likely_mat likelySrc = likelyFromOpenCVMat(srcCV);
                if (!(likelySrc->type & likely_floating) && ((likelySrc->type & likely_depth) <= 16))
                    likelySrc->type |= likely_saturated; // Follow OpenCV's saturation convention
                vector<likely_const_mat> likelyArgs = additionalArguments(type);
                likelyArgs.insert(likelyArgs.begin(), likelySrc);

                const likely_const_mat typeString = likely_type_to_string(type);
                printf("%s \t%s \t%d \t%s\t", name(), typeString->data, size, BenchmarkMulticore ? "m" : "s");
                likely_release_mat(typeString);
                testCorrectness(reinterpret_cast<likely_mat (*)(const likely_const_mat*)>(f), srcCV, likelyArgs.data());

                if (!BenchmarkTest) {
                    const Speed baseline = testBaselineSpeed(srcCV);
                    const Speed likely = testLikelySpeed(reinterpret_cast<likely_mat (*)(const likely_const_mat*)>(f), likelyArgs.data());
                    printf("%-8.3g \t%.3gx\n", double(likely.iterations), likely.Hz/baseline.Hz);
                } else {
                    printf("\n");
                }

                for (const likely_const_mat &likelyArg : likelyArgs)
                    likely_release_mat(likelyArg);
            }
        }

        likely_release_env(env);
    }

    static void runFile(const char *fileName)
    {
        const likely_file_type file_type = likely_guess_file_type(fileName);
        const likely_const_mat source = likely_read(fileName, file_type, likely_text);
        checkRead(source, fileName);

        printf("%s \t", fileName);
        const likely_const_env parent = likely_standard(likely_jit(false), NULL);
        likely_release_env(likely_lex_parse_and_eval(source->data, file_type, parent));

        if (BenchmarkTest) {
            printf("\n");
        } else {
            clock_t startTime, endTime;
            int iter = 0;
            startTime = endTime = clock();
            while ((endTime-startTime) / CLOCKS_PER_SEC < TestSeconds) {
                likely_release_env(likely_lex_parse_and_eval(source->data, file_type, parent));
                endTime = clock();
                iter++;
            }
            Speed speed(iter, startTime, endTime);
            printf("%.2e\n", speed.Hz);
        }

        likely_release_env(parent);
        likely_release_mat(source);
    }

protected:
    virtual const char *name() const = 0;
    virtual Mat computeBaseline(const Mat &src) const = 0;
    virtual int additionalParameters() const = 0;
    virtual bool color() const { return false; }

    virtual vector<likely_const_mat> additionalArguments(likely_type) const
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

    void testCorrectness(likely_mat (*f)(const likely_const_mat*), const Mat &srcCV, const likely_const_mat *srcLikely) const
    {
        Mat dstOpenCV = computeBaseline(srcCV);
        const likely_const_mat dstLikely = f(srcLikely);

        Mat errorMat = abs(likelyToOpenCVMat(dstLikely) - dstOpenCV);
        errorMat.convertTo(errorMat, CV_32F);
        dstOpenCV.convertTo(dstOpenCV, CV_32F);
        errorMat = errorMat / (dstOpenCV + ErrorTolerance); // Normalize errors
        threshold(errorMat, errorMat, ErrorTolerance, 1, THRESH_BINARY);

        if (norm(errorMat, NORM_L1) > 0) {
            const likely_const_mat cvLikely = likelyFromOpenCVMat(dstOpenCV);
            stringstream errorLocations;
            errorLocations << "input\topencv\tlikely\trow\tcolumn\tchannel\n";
            int errors = 0;
            for (int i=0; i<dstOpenCV.rows; i++)
                for (int j=0; j<dstOpenCV.cols; j++)
                    for (int k=0; k<dstOpenCV.channels(); k++)
                        if (errorMat.at<float>(i, j, k) == 1) {
                            stringstream src;
                            if ((dstOpenCV.channels() == 1) && (srcCV.channels() > 1)) {
                                src << "(";
                                for (int c=0; c<srcCV.channels(); c++) {
                                    src << likely_get_element(srcLikely[0], c, j, i, 0);
                                    if (c < srcCV.channels() - 1)
                                        src << " ";
                                }
                                src << ")";
                            } else {
                                src << likely_get_element(srcLikely[0], k, j, i, 0);
                            }
                            const double cv  = likely_get_element(cvLikely    , k, j, i, 0);
                            const double dst = likely_get_element(dstLikely   , k, j, i, 0);
                            if (errors < 100) errorLocations << src.str() << "\t" << cv << "\t" << dst << "\t" << i << "\t" << j << "\t" << k << "\n";
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

    Speed testBaselineSpeed(const Mat &src) const
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

template <int const N = 0>
struct Test : public TestBase
{
    int additionalParameters() const
    {
        return N;
    }
};

class BinaryThreshold : public Test<2>
{
    const char *name() const
    {
        return "binary-threshold";
    }

    vector<likely_const_mat> additionalArguments(likely_type type) const
    {
        vector<likely_const_mat> args;
        const double thresh = 127;
        const double maxval = 1;
        args.push_back(likely_scalar(type, &thresh, 1));
        args.push_back(likely_scalar(type, &maxval, 1));
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

class FusedMultiplyAdd : public Test<2>
{
    const char *name() const
    {
        return "fused-multiply-add";
    }

    vector<likely_const_mat> additionalArguments(likely_type type) const
    {
        const likely_type scalarType = ((type & likely_depth) <= 32) ? likely_f32 : likely_f64;
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

class MinMaxLoc : public Test<>
{
    const char *name() const
    {
        return "min-max-loc";
    }

    Mat computeBaseline(const Mat &src) const
    {
        Mat result(2, 3, CV_MAKETYPE(CV_64F, src.channels()));
        vector<Mat> mv;
        split(src, mv);

        for (int i=0; i<int(mv.size()); i++) {
            double minVal, maxVal;
            Point minLoc, maxLoc;
            minMaxLoc(mv[i], &minVal, &maxVal, &minLoc, &maxLoc);

            result.at<double>(0, 0, i) = minVal;
            result.at<double>(0, 1, i) = minLoc.x;
            result.at<double>(0, 2, i) = minLoc.y;
            result.at<double>(1, 0, i) = maxVal;
            result.at<double>(1, 1, i) = maxLoc.x;
            result.at<double>(1, 2, i) = maxLoc.y;
        }

        return result;
    }
};

class ConvertGrayscale : public Test<>
{
    const char *name() const
    {
        return "convert-grayscale";
    }

    bool color() const
    {
        return true;
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

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    // Print to console immediately
    setbuf(stdout, NULL);

    if (!BenchmarkFile.empty()) {
        TestBase::runFile(BenchmarkFile.getValue().c_str());
    } else {
        const time_t now = time(0);
        char dateTime[80];
        strftime(dateTime, sizeof(dateTime), "%Y-%m-%d.%X", localtime(&now));
        puts(dateTime);
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
        puts("To reproduce the following results, run the `benchmark` application, included in a build of Likely, from the root of the Likely repository.");
        puts("");
        puts("Function \t\tType \tSize \tExec \tIter \t\tSpeedup");

        likely_settings settings = likely_jit(false);
        settings.multicore = BenchmarkMulticore;
        settings.verbose = BenchmarkVerbose;
        settings.opt_level = BenchmarkHuman ? 2 : 3;
        settings.size_level = BenchmarkHuman ? 2 : 0;
        settings.unroll_loops = !BenchmarkHuman;
        settings.vectorize_loops = !BenchmarkHuman;
        const likely_const_env parent = likely_standard(settings, NULL);

        BinaryThreshold().run(parent);
        ConvertGrayscale().run(parent);
        FusedMultiplyAdd().run(parent);
        MinMaxLoc().run(parent);
        likely_release_env(parent);
    }

    likely_shutdown();
    return EXIT_SUCCESS;
}
