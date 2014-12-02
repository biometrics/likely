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
 * Its source code in <tt>src/like.cpp</tt> is reproduced below:
 * \snippet src/like.cpp console_interpreter_compiler implementation.
 */

//! [console_interpreter_compiler implementation.]
#include <likely.h>

// Until Microsoft implements snprintf
#if _MSC_VER
#define snprintf _snprintf
#endif

using namespace llvm;
using namespace std;

static cl::opt<string> input(cl::Positional, cl::desc("<source_file>"));
static cl::opt<string> output(cl::Positional, cl::desc("<object_file>"));
static cl::opt<bool> command("command", cl::desc("Treat <source_file> as a command instead of a file"));
static cl::alias     commandA("c", cl::desc("Alias for -command"), cl::aliasopt(command));
static cl::opt<string> render("render", cl::desc("%d-formatted file to render matrix output to"));
static cl::alias       renderA("r", cl::desc("Alias for -render"), cl::aliasopt(render));
static cl::opt<string> assert_("assert", cl::desc("Confirm the output equals the specified value"));
static cl::alias     assertA("A", cl::desc("Alias for -assert"), cl::aliasopt(assert_));
static cl::opt<bool> ast("ast", cl::desc("Print abstract syntax tree"));
static cl::alias     astA("a", cl::desc("Alias for -ast"), cl::aliasopt(ast));
static cl::opt<bool> show("show", cl::desc("Show matrix output in a window"));
static cl::alias     showA("s", cl::desc("Alias for -show"), cl::aliasopt(show));
static cl::opt<bool> quiet("quiet", cl::desc("Don't show matrix output"));
static cl::alias     quietA("q", cl::desc("Alias for -quiet"), cl::aliasopt(quiet));
static cl::opt<bool> parallel("parallel" , cl::desc("Compile parallel kernels"));
static cl::alias     parallelA("p", cl::desc("Alias for -parallel"), cl::aliasopt(parallel));
static cl::opt<bool> verbose("verbose" , cl::desc("Verbose compiler output"));
static cl::alias     verboseA("v", cl::desc("Alias for -verbose"), cl::aliasopt(verbose));

cl::OptionCategory LLVMCat("LLVM Options", "These control the behavior of the internal LLVM compiler.");
static cl::opt<bool> OptLevelO0("O0", cl::desc("No optimizations. Similar to clang -O1"), cl::cat(LLVMCat));
static cl::opt<bool> OptLevelO1("O1", cl::desc("Optimization level 1. Similar to clang -O1"), cl::cat(LLVMCat));
static cl::opt<bool> OptLevelO2("O2", cl::desc("Optimization level 2. Similar to clang -O2"), cl::cat(LLVMCat));
static cl::opt<bool> OptLevelOs("Os", cl::desc("Like -O2 with extra optimizations for size. Similar to clang -Os"), cl::cat(LLVMCat));
static cl::opt<bool> OptLevelOz("Oz", cl::desc("Like -Os but reduces code size further. Similar to clang -Oz"), cl::cat(LLVMCat));
static cl::opt<bool> OptLevelO3("O3", cl::desc("Optimization level 3. Similar to clang -O3"), cl::cat(LLVMCat));
static cl::opt<bool> DisableLoopUnrolling("disable-loop-unrolling", cl::desc("Disable loop unrolling in all relevant passes"), cl::cat(LLVMCat));
static cl::opt<bool> DisableLoopVectorization("disable-loop-vectorization", cl::desc("Disable the loop vectorization pass"), cl::cat(LLVMCat));

static void checkOrPrintAndRelease(likely_const_mat input)
{
    if (!input)
        return;
    assert(likely_is_string(input));
    if (assert_.getValue().empty()) {
        printf("%s\n", input->data);
    } else {
        const size_t len = input->channels - 1;
        likely_assert((len <= assert_.getValue().size()) && !strncmp(input->data, assert_.getValue().c_str(), len),
                      "expected: %s\n        but got: %s", assert_.getValue().c_str(), input->data);
        assert_.setValue(assert_.getValue().substr(len));
    }
    likely_release_mat(input);
}

static void renderCallback(likely_const_env env, void *)
{
    if (env->type & likely_environment_definition)
        return;

    static int index = 0;
    const int bufferSize = 128;
    char fileName[bufferSize];
    snprintf(fileName, bufferSize, render.getValue().c_str(), index++);
    const likely_mat rendered = likely_render(likely_result(env->expr), NULL, NULL);
    likely_release_mat(likely_write(rendered, fileName));
    likely_release_mat(rendered);
}

static void showCallback(likely_const_env env, void *)
{
    if (env->type & likely_environment_definition)
        return;

    const likely_const_mat rendered = likely_render(likely_result(env->expr), NULL, NULL);
    if (assert_.getValue().empty()) {
        likely_release_mat(likely_show(rendered, likely_symbol(env->ast)));
    } else {
        const likely_const_mat baseline = likely_read(assert_.getValue().c_str(), likely_file_guess, likely_image);
        likely_assert_approximate(baseline, rendered, 0.03f /* arbitrary threshold */);
        likely_release_mat(baseline);
        assert_.setValue(string());
    }
    likely_release_mat(rendered);
}

static void quietCallback(likely_const_env, void *)
{
    return;
}

static void printCallback(likely_const_env env, void *)
{
    if (env->type & likely_environment_definition)
        return;
    checkOrPrintAndRelease(likely_to_string(likely_result(env->expr)));
}

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    likely_eval_callback evalCallback;
    if (!render.getValue().empty()) evalCallback = renderCallback;
    else if (show)  evalCallback = showCallback;
    else if (quiet) evalCallback = quietCallback;
    else            evalCallback = printCallback;

    likely_settings settings;
    settings.opt_level = OptLevelO3 ? 3 : ((OptLevelO2 || OptLevelOs || OptLevelOz) ? 2 : (OptLevelO1 ? 1 : (OptLevelO0 ? 0 : (output.empty() ? 3 : 0))));
    settings.size_level = OptLevelOz ? 2 : (OptLevelOs ? 1 : 0);
    settings.parallel = parallel;
    settings.heterogeneous = false;
    settings.unroll_loops = !DisableLoopUnrolling;
    settings.vectorize_loops = !DisableLoopVectorization;
    settings.verbose = verbose;

    likely_env parent = likely_standard(settings, output.empty() ? NULL /* JIT */ : output.c_str() /* Offline */);
    if (parallel)
        parent->type |= likely_environment_parallel;

    if (input.empty()) {
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
        if (command) {
            type = likely_file_lisp;
            code = likely_string(input.c_str());
        } else {
            type = likely_guess_file_type(input.c_str());
            code = likely_read(input.c_str(), type, likely_text);
            likely_assert(code != NULL, "failed to read: %s", input.c_str());
        }

        const likely_ast parsed = likely_lex_and_parse(code->data, type);
        likely_release_mat(code);
        if (ast) {
            for (size_t i=0; i<parsed->num_atoms; i++)
                checkOrPrintAndRelease(likely_ast_to_string(parsed->atoms[i]));
        } else {
            const likely_const_env env = likely_eval(parsed, parent, evalCallback, NULL);
            if (!env->expr && env->ast) {
                const likely_const_mat statement = likely_ast_to_string(env->ast);
                likely_assert(false, "error evaluating: %s", statement->data);
                likely_release_mat(statement);
            }
            likely_release_env(env);
        }
        likely_release_ast(parsed);
    }

    likely_assert(assert_.getValue().empty(), "unreached assertion: %s", assert_.getValue().data());

    assert(parent->ref_count == 1);
    likely_release_env(parent);
    likely_shutdown();
    return EXIT_SUCCESS;
}
//! [console_interpreter_compiler implementation.]
