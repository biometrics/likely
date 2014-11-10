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

static cl::opt<bool> BenchmarkTest    ("test"     , cl::desc("Run tests for correctness only"));
static cl::opt<bool> BenchmarkParallel("parallel" , cl::desc("Compile parallel kernels"));
static cl::opt<string> BenchmarkFile    ("file"    , cl::desc("Benchmark the specified file only"), cl::value_desc("filename"));
static cl::opt<string> BenchmarkFunction("function", cl::desc("Benchmark the specified function only"), cl::value_desc("string"));

static void checkRead(const void *data, const char *fileName)
{
    likely_assert(data != NULL, "failed to read \"%s\", did you forget to run 'benchmark' from the root of the repository?", fileName);
}

static Mat generateData(int rows, int columns, likely_type type)
{
    static Mat m;
    if (!m.data) {
        m = imread("data/misc/lenna.tiff");
        checkRead(m.data, "data/misc/lenna.tiff");
        cvtColor(m, m, CV_BGR2GRAY);
    }

    Mat n;
    resize(m, n, Size(columns, rows), 0, 0, INTER_NEAREST);
    n.convertTo(n, likelyToOpenCVDepth(type));
    return n;
}

struct Test
{
    void run(likely_const_env env) const
    {
        if (!BenchmarkFunction.empty() && (name() != BenchmarkFunction))
            return;

        const likely_const_env lookup = likely_lookup(env, name());
        assert(lookup && lookup->expr);
        void * const f = likely_compile(lookup->expr, NULL, 0);

        for (likely_type type : types()) {
            for (int size : sizes()) {
                if (BenchmarkTest && (size != 256)) continue;

                // Generate input matrix
                Mat srcCV = generateData(size, size, type);
                likely_mat srcLikely = fromCvMat(srcCV);

                likely_mat typeString = likely_type_to_string(type);
                printf("%s \t%s \t%d \t%s\t", name(), typeString->data, size, BenchmarkParallel ? "P" : "S");
                likely_release_mat(typeString);
                testCorrectness(reinterpret_cast<likely_mat (*)(likely_const_mat)>(f), srcCV, srcLikely);

                if (BenchmarkTest) {
                    printf("\n");
                    continue;
                }

                Speed baseline = testBaselineSpeed(srcCV);
                Speed likely = testLikelySpeed(reinterpret_cast<likely_mat (*)(likely_const_mat)>(f), srcLikely);
                printf("%-8.3g \t%.3gx\n", double(likely.iterations), likely.Hz/baseline.Hz);
            }
        }
    }

    static void runFile(const char *fileName)
    {
        const likely_file_type type = likely_guess_file_type(fileName);
        likely_const_mat source = likely_read(fileName, type);
        checkRead(source, fileName);

        printf("%s \t", fileName);
        likely_ast ast = likely_lex_and_parse(source->data, type);
        likely_release_mat(source);
        likely_env parent = likely_standard(NULL);
        likely_release_env(likely_eval(ast, parent, NULL, NULL));

        if (BenchmarkTest) {
            printf("\n");
        } else {
            clock_t startTime, endTime;
            int iter = 0;
            startTime = endTime = clock();
            while ((endTime-startTime) / CLOCKS_PER_SEC < TestSeconds) {
                likely_release_env(likely_eval(ast, parent, NULL, NULL));
                endTime = clock();
                iter++;
            }
            Speed speed(iter, startTime, endTime);
            printf("%.2e\n", speed.Hz);
        }

        likely_release_env(parent);
        likely_release_ast(ast);
    }

protected:
    virtual const char *name() const = 0;
    virtual Mat computeBaseline(const Mat &src) const = 0;
    virtual vector<likely_type> types() const
    {
        static vector<likely_type> types;
        if (types.empty()) {
            types.push_back(likely_u8);
            types.push_back(likely_u16);
            types.push_back(likely_i32);
            types.push_back(likely_f32);
            types.push_back(likely_f64);
        }
        return types;
    }

    virtual vector<int> sizes() const
    {
        static vector<int> sizes;
        if (sizes.empty()) {
            sizes.push_back(8);
            sizes.push_back(16);
            sizes.push_back(32);
            sizes.push_back(64);
            sizes.push_back(128);
            sizes.push_back(256);
            sizes.push_back(512);
            sizes.push_back(1024);
            sizes.push_back(2048);
        }
        return sizes;
    }

    virtual bool serial() const { return true; }
    virtual bool parallel() const { return true; }

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
        likely_mat m = likelyFromOpenCVMat(src);
        if (!(m->type & likely_floating) && ((m->type & likely_depth) <= 16))
            m->type |= likely_saturated;
        return m;
    }

    void testCorrectness(likely_mat (*f)(likely_const_mat), const Mat &srcCV, const likely_const_mat srcLikely) const
    {
        Mat dstOpenCV = computeBaseline(srcCV);
        likely_const_mat dstLikely = f(srcLikely);

        Mat errorMat = abs(likelyToOpenCVMat(dstLikely) - dstOpenCV);
        errorMat.convertTo(errorMat, CV_32F);
        dstOpenCV.convertTo(dstOpenCV, CV_32F);
        errorMat = errorMat / (dstOpenCV + ErrorTolerance); // Normalize errors
        threshold(errorMat, errorMat, ErrorTolerance, 1, THRESH_BINARY);
        int errors = (int) norm(errorMat, NORM_L1);
        if (errors > 0) {
            likely_const_mat cvLikely = fromCvMat(dstOpenCV);
            stringstream errorLocations;
            errorLocations << "input\topencv\tlikely\trow\tcolumn\n";
            errors = 0;
            for (int i=0; i<srcCV.rows; i++)
                for (int j=0; j<srcCV.cols; j++)
                    if (errorMat.at<float>(i, j) == 1) {
                        const double src = likely_get_element(srcLikely, 0, j, i, 0);
                        const double cv  = likely_get_element(cvLikely,  0, j, i, 0);
                        const double dst = likely_get_element(dstLikely, 0, j, i, 0);
                        if (errors < 100) errorLocations << src << "\t" << cv << "\t" << dst << "\t" << i << "\t" << j << "\n";
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
        return Test::Speed(iter, startTime, endTime);
    }

    Speed testLikelySpeed(likely_mat (*f)(likely_const_mat), const likely_const_mat srcLikely) const
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
        return Test::Speed(iter, startTime, endTime);
    }
};

class fmaTest : public Test {
    const char *name() const { return "fused-multiply-add"; }
    Mat computeBaseline(const Mat &src) const { Mat dst; src.convertTo(dst, src.depth() == CV_64F ? CV_64F : CV_32F, 2, 3); return dst; }
};

class thresholdTest : public Test {
    const char *name() const { return "binary-threshold"; }
    Mat computeBaseline(const Mat &src) const { Mat dst; threshold(src, dst, 127, 1, THRESH_BINARY); return dst; }
    vector<likely_type> types() const { vector<likely_type> types; types.push_back(likely_u8); types.push_back(likely_f32); return types; }
};

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);
    likely_initialize(3, 0, true);

    // Print to console immediately
    setbuf(stdout, NULL);

    if (!BenchmarkFile.empty()) {
        Test::runFile(BenchmarkFile.getValue().c_str());
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
        puts("    Exec: (S)erial or (P)arallel");
        puts("    Iter: Times Likely function was run in one second");
        puts(" Speedup: Execution speed of Likely relative to OpenCV");
        puts("");
        puts("To reproduce the following results, run the `benchmark` application, included in a build of Likely, from the root of the Likely repository.");
        puts("");
        puts("Function \t\tType \tSize \tExec \tIter \t\tSpeedup");

        const likely_const_mat source = likely_read("library/benchmark.md", likely_file_gfm);
        checkRead(source, "library/benchmark.md");
        const likely_ast ast = likely_lex_and_parse(source->data, likely_file_gfm);
        likely_release_mat(source);
        const likely_env parent = likely_standard(NULL);
        if (BenchmarkParallel)
            parent->type |= likely_environment_parallel;
        const likely_const_env env = likely_eval(ast, parent, NULL, NULL);
        assert(env->expr);
        likely_release_env(parent);
        likely_release_ast(ast);

        fmaTest().run(env);
        thresholdTest().run(env);
    }

    likely_shutdown();
    return EXIT_SUCCESS;
}

