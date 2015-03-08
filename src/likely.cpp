/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2014 Joshua C. Klontz                                           *
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <llvm/Support/CommandLine.h>

/*!
 * \page console_interpreter_compiler Console-Interpreter-Compiler
 * \brief Documentation and source code for the \c likely console-interpreter-compiler command line application.
 *
 * This is the standard tool for dealing with Likely source code, and supports for three primary use cases:
\verbatim
$ likely                                       # enter a read-evaluate-print-loop console
$ likely <source_file_or_string>               # interpret <source_file_or_string> as a script
$ likely <source_file_or_string> <object_file> # compile <source_file_or_string> into an <object_file>
\endverbatim
 *
 * This application is a good entry point for learning about the Likely API.
 * Its source code in <tt>src/likely.cpp</tt> is reproduced below:
 * \snippet src/likely.cpp console_interpreter_compiler implementation.
 */

//! [console_interpreter_compiler implementation.]
#include <likely.h>

// Until Microsoft implements snprintf
#if _MSC_VER
#define snprintf _snprintf
#endif

using namespace llvm;
using namespace std;

static cl::opt<string> LikelyInput(cl::Positional, cl::desc("<source_file>"));
static cl::opt<string> LikelyOutput(cl::Positional, cl::desc("<object_file>"));
static cl::opt<bool> LikelyCommand("command", cl::desc("Treat <source_file> as a command instead of a file"));
static cl::alias     LikelyCommandA("c", cl::desc("Alias for -command"), cl::aliasopt(LikelyCommand));
static cl::opt<string> LikelyEnsure("ensure", cl::desc("Confirm the output equals the specified value"));
static cl::alias       LikelyEnsureA("e", cl::desc("Alias for -ensure"), cl::aliasopt(LikelyEnsure));
static cl::opt<bool> LikelyAst("ast", cl::desc("Print abstract syntax tree"));
static cl::alias     LikelyAstA("a", cl::desc("Alias for -ast"), cl::aliasopt(LikelyAst));
static cl::opt<bool> LikelyVerbose("verbose", cl::desc("Verbose compiler output"));
static cl::alias     LikelyVerboseA("v", cl::desc("Alias for -verbose"), cl::aliasopt(LikelyVerbose));
static cl::opt<bool> LikelyCtfeInherit("ctfe-inherit", cl::desc("Compile time function evaluation should inherit static compiler settings"));
static cl::alias     LikelyCtfeInheritA("i", cl::desc("Alias for -ctfe-inherit"), cl::aliasopt(LikelyCtfeInherit));
static cl::opt<bool> LikelyExamine("examine", cl::desc("Alias for -q -v -Oz -disable-loop-unrolling -disable-loop-vectorization"));
static cl::alias     LikelyExamineA("ex", cl::desc("Alias for -examine"), cl::aliasopt(LikelyExamine));

cl::OptionCategory ArchitectureCategory("Architecture");
static cl::opt<bool> LikelyMulticore("multi-core" , cl::desc("Compile multi-core kernels"), cl::cat(ArchitectureCategory));
static cl::alias     LikelyMulticoreA("m", cl::desc("Alias for -multi-core"), cl::cat(ArchitectureCategory), cl::aliasopt(LikelyMulticore));
static cl::opt<bool> LikelyHeterogeneous("heterogeneous" , cl::desc("Compile heterogeneous kernels"), cl::cat(ArchitectureCategory));
static cl::alias     LikelyHeterogeneousA("h", cl::desc("Alias for -heterogeneous"), cl::cat(ArchitectureCategory), cl::aliasopt(LikelyHeterogeneous));

cl::OptionCategory OptimizationsCategory("Optimizations");
static cl::opt<bool> LikelyO0("O0", cl::desc("No optimizations"), cl::cat(OptimizationsCategory));
static cl::opt<bool> LikelyO1("O1", cl::desc("Optimization level 1"), cl::cat(OptimizationsCategory));
static cl::opt<bool> LikelyO2("O2", cl::desc("Optimization level 2"), cl::cat(OptimizationsCategory));
static cl::opt<bool> LikelyO3("O3", cl::desc("Optimization level 3"), cl::cat(OptimizationsCategory));
static cl::opt<bool> LikelyOs("Os", cl::desc("Like -O2 but with extra optimizations for size"), cl::cat(OptimizationsCategory));
static cl::opt<bool> LikelyOz("Oz", cl::desc("Like -Os but reduces code size further"), cl::cat(OptimizationsCategory));
static cl::opt<bool> LikelyDisableLoopUnrolling("disable-loop-unrolling", cl::desc("Disable loop unrolling in all relevant passes"), cl::cat(OptimizationsCategory));
static cl::opt<bool> LikelyDisableLoopVectorization("disable-loop-vectorization", cl::desc("Disable the loop vectorization pass"), cl::cat(OptimizationsCategory));

cl::OptionCategory PrintingCategory("Printing");
static cl::opt<string> LikelyRender ("render", cl::desc("%d-formatted file to render matrix output to"), cl::cat(PrintingCategory));
static cl::alias       LikelyRenderA("r", cl::desc("Alias for -render"), cl::cat(PrintingCategory), cl::aliasopt(LikelyRender));
static cl::opt<bool> LikelyShow("show", cl::desc("Show matrix output in a window"), cl::cat(PrintingCategory));
static cl::alias     LikelyShowA("s", cl::desc("Alias for -show"), cl::cat(PrintingCategory), cl::aliasopt(LikelyShow));
static cl::opt<bool> LikelyQuiet("quiet", cl::desc("Don't print matrix output"), cl::cat(PrintingCategory));
static cl::alias     LikelyQuietA("q", cl::desc("Alias for -quiet"), cl::cat(PrintingCategory), cl::aliasopt(LikelyQuiet));

static void checkOrPrintAndRelease(likely_const_mat input)
{
    if (!input)
        return;
    assert(likely_is_string(input));
    if (LikelyEnsure.getValue().empty()) {
        printf("%s\n", input->data);
    } else {
        const size_t len = input->channels - 1;
        likely_ensure((len <= LikelyEnsure.getValue().size()) && !strncmp(input->data, LikelyEnsure.getValue().c_str(), len),
                      "expected: %s\n        but got: %s", LikelyEnsure.getValue().c_str(), input->data);
        LikelyEnsure.setValue(LikelyEnsure.getValue().substr(len));
    }
    likely_release_mat(input);
}

static void renderCallback(likely_const_env env, void *)
{
    if (likely_is_definition(env->ast))
        return;

    static int index = 0;
    const int bufferSize = 128;
    char fileName[bufferSize];
    snprintf(fileName, bufferSize, LikelyRender.getValue().c_str(), index++);
    const likely_mat rendered = likely_render(likely_result(env->expr), NULL, NULL);
    likely_release_mat(likely_write(rendered, fileName));
    likely_release_mat(rendered);
}

static void showCallback(likely_const_env env, void *)
{
    if (likely_is_definition(env->ast))
        return;

    const likely_const_mat rendered = likely_render(likely_result(env->expr), NULL, NULL);
    if (LikelyEnsure.getValue().empty()) {
        likely_release_mat(likely_show(rendered, likely_symbol(env->ast)));
    } else {
        const likely_const_mat baseline = likely_read(LikelyEnsure.getValue().c_str(), likely_file_guess, likely_image);
        likely_ensure_approximate(baseline, rendered, 0.03f /* arbitrary threshold */);
        likely_release_mat(baseline);
        LikelyEnsure.setValue(string());
    }
    likely_release_mat(rendered);
}

static void quietCallback(likely_const_env, void *)
{
    return;
}

static void printCallback(likely_const_env env, void *)
{
    if (likely_is_definition(env->ast))
        return;
    checkOrPrintAndRelease(likely_to_string(likely_result(env->expr)));
}

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    if (LikelyExamine) {
        LikelyVerbose.setValue(true);
        LikelyQuiet.setValue(true);
        LikelyCtfeInherit.setValue(true);
        LikelyOz.setValue(true);
        LikelyDisableLoopUnrolling.setValue(true);
        LikelyDisableLoopVectorization.setValue(true);
    }

    likely_eval_callback evalCallback;
    if (!LikelyRender.getValue().empty()) evalCallback = renderCallback;
    else if (LikelyShow)  evalCallback = showCallback;
    else if (LikelyQuiet) evalCallback = quietCallback;
    else                  evalCallback = printCallback;

    likely_settings settings;
    settings.opt_level = LikelyO3 ? 3 : ((LikelyO2 || LikelyOs || LikelyOz) ? 2 : (LikelyO1 ? 1 : (LikelyO0 ? 0 : (LikelyOutput.empty() ? 3 : 0))));
    settings.size_level = LikelyOz ? 2 : (LikelyOs ? 1 : 0);
    settings.multicore = LikelyMulticore;
    settings.heterogeneous = LikelyHeterogeneous;
    settings.unroll_loops = !LikelyDisableLoopUnrolling;
    settings.vectorize_loops = !LikelyDisableLoopVectorization;
    settings.verbose = LikelyVerbose;
    settings.ctfe_inherit = LikelyCtfeInherit;

    likely_const_env parent = likely_standard(settings, LikelyOutput.empty() ? NULL /* JIT */ : LikelyOutput.c_str() /* Offline */);

    if (LikelyInput.empty()) {
        // console
        cout << "Likely\n";
        while (true) {
            cout << "> ";
            string line;
            getline(cin, line);
            const likely_env env = likely_lex_parse_and_eval(line.c_str(), likely_file_lisp, parent);
            if (!env->expr) {
                likely_release_env(env);
            } else {
                likely_release_env(parent);
                parent = env;
                evalCallback(env, (void*) true);
            }
        }
    } else {
        // interpreter or compiler
        likely_file_type type;
        likely_mat code;
        if (LikelyCommand) {
            type = likely_file_lisp;
            code = likely_string(LikelyInput.c_str());
        } else {
            type = likely_guess_file_type(LikelyInput.c_str());
            code = likely_read(LikelyInput.c_str(), type, likely_text);
            likely_ensure(code != NULL, "failed to read: %s", LikelyInput.c_str());
        }

        const likely_ast parsed = likely_lex_and_parse(code->data, type);
        likely_release_mat(code);
        if (LikelyAst) {
            for (size_t i=0; i<parsed->num_atoms; i++)
                checkOrPrintAndRelease(likely_ast_to_string(parsed->atoms[i]));
        } else {
            const likely_const_env env = likely_eval(parsed, parent, evalCallback, NULL);
            if (!env->expr && env->ast) {
                const likely_const_mat statement = likely_ast_to_string(env->ast);
                likely_ensure(false, "error evaluating: %s", statement->data);
                likely_release_mat(statement);
            }
            likely_release_env(env);
        }
        likely_release_ast(parsed);
    }

    likely_ensure(LikelyEnsure.getValue().empty(), "unreached assertion: %s", LikelyEnsure.getValue().data());

    assert(parent->ref_count == 1);
    likely_release_env(parent);
    likely_shutdown();
    return EXIT_SUCCESS;
}
//! [console_interpreter_compiler implementation.]
