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

static cl::opt<string> input(cl::Positional, cl::desc("<source_file_or_string>"), cl::init(""));
static cl::opt<string> output(cl::Positional, cl::desc("<object_file>"), cl::init(""));
static cl::opt<string> render("render", cl::desc("%d-formatted file to render matrix output to"));
static cl::opt<string> assert_("assert", cl::desc("Confirm the output equals the specified value"));
static cl::opt<bool> ast("ast", cl::desc("Print abstract syntax tree"));
static cl::opt<bool> show("show", cl::desc("Show matrix output in a window"));
static cl::opt<bool> quiet("quiet", cl::desc("Don't show matrix output"));
static cl::opt<bool> parallel("parallel" , cl::desc("Compile parallel kernels"));

// These variables mirror LLVM's `opt`
static cl::opt<bool> OptLevelO1("O1", cl::desc("Optimization level 1. Similar to clang -O1"));
static cl::opt<bool> OptLevelO2("O2", cl::desc("Optimization level 2. Similar to clang -O2"));
static cl::opt<bool> OptLevelOs("Os", cl::desc("Like -O2 with extra optimizations for size. Similar to clang -Os"));
static cl::opt<bool> OptLevelOz("Oz", cl::desc("Like -Os but reduces code size further. Similar to clang -Oz"));
static cl::opt<bool> OptLevelO3("O3", cl::desc("Optimization level 3. Similar to clang -O3"));
static cl::opt<bool> DisableLoopVectorization("disable-loop-vectorization", cl::desc("Disable the loop vectorization pass"), cl::init(false));

static void checkOrPrintAndRelease(likely_const_mat input)
{
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

static void replRender(likely_const_env env, void *)
{
    if (env->type & likely_environment_definition)
        return;

    static int index = 0;
    const int bufferSize = 128;
    char fileName[bufferSize];
    snprintf(fileName, bufferSize, render.getValue().c_str(), index++);
    likely_mat rendered = likely_render(likely_result(env), NULL, NULL);
    likely_release_mat(likely_write(rendered, fileName));
    likely_release_mat(rendered);
}

static void replShow(likely_const_env env, void *)
{
    if (env->type & likely_environment_definition)
        return;

    if (assert_.getValue().empty()) {
        likely_show(likely_result(env), likely_symbol(env->ast));
    } else {
        likely_mat rendered = likely_render(likely_result(env), NULL, NULL);
        likely_mat baseline = likely_read(assert_.getValue().c_str(), likely_file_guess);
        likely_assert(rendered->channels == baseline->channels, "expected: %d channels, got: %d", baseline->channels, rendered->channels);
        likely_assert(rendered->columns  == baseline->columns , "expected: %d columns, got: %d" , baseline->columns , rendered->columns);
        likely_assert(rendered->rows     == baseline->rows    , "expected: %d rows, got: %d"    , baseline->rows    , rendered->rows);
        likely_assert(rendered->frames   == baseline->frames  , "expected: %d frames, got: %d"  , baseline->frames  , rendered->frames);
        const size_t elements = size_t(rendered->channels) * size_t(rendered->columns) * size_t(rendered->rows) * size_t(rendered->frames);
        size_t delta = 0;
        for (size_t i=0; i<elements; i++)
            delta += abs(int(rendered->data[i]) - int(baseline->data[i]));
        likely_release_mat(rendered);
        likely_release_mat(baseline);
        likely_assert(delta < 2*elements /* arbitrary threshold */, "average delta: %g", float(delta) / float(elements));
        assert_.setValue(string());
    }
}

static void replQuiet(likely_const_env, void *)
{
    return;
}

static void replPrint(likely_const_env env, void *)
{
    if (env->type & likely_environment_definition)
        return;
    checkOrPrintAndRelease(likely_to_string(likely_result(env)));
}

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    likely_repl_callback repl_callback;
    if (!render.getValue().empty()) repl_callback = replRender;
    else if (show)  repl_callback = replShow;
    else if (quiet) repl_callback = replQuiet;
    else            repl_callback = replPrint;

    likely_initialize(OptLevelO3 ? 3 : ((OptLevelO2 || OptLevelOs || OptLevelOz) ? 2 : (OptLevelO1 ? 1 : (output.empty() ? 3 : 0))),
                      OptLevelOz ? 2 : (OptLevelOs ? 1 : 0),
                      !DisableLoopVectorization);

    likely_env parent = likely_standard(output.empty() ? NULL /* JIT */ : output.c_str() /* Offline */);
    if (parallel)
        parent->type |= likely_environment_parallel;

    if (input.empty()) {
        // console
        cout << "Likely\n";
        while (true) {
            cout << "> ";
            string line;
            getline(cin, line);
            likely_ast ast = likely_lex_and_parse(line.c_str(), likely_file_lisp);
            likely_env env = likely_eval(ast->atoms[0], parent);
            likely_release_ast(ast);
            if (!env->expr) {
                likely_release_env(env);
            } else {
                likely_release_env(parent);
                parent = env;
                repl_callback(env, (void*) true);
            }
        }
    } else {
        // interpreter or compiler
        likely_file_type type = likely_guess_file_type(input.c_str());
        likely_mat code = likely_read(input.c_str(), type);
        if (!code) {
            type = likely_file_lisp;
            code = likely_string(input.c_str());
        }

        if (ast) {
            likely_ast parsed = likely_lex_and_parse(code->data, type);
            for (size_t i=0; i<parsed->num_atoms; i++)
                checkOrPrintAndRelease(likely_ast_to_string(parsed->atoms[i]));
            likely_release_ast(parsed);
        } else {
            likely_ast ast = likely_lex_and_parse(code->data, type);
            likely_const_env env = likely_repl(ast, parent, repl_callback, NULL);
            if (!env->expr && env->ast) {
                likely_const_mat statement = likely_ast_to_string(env->ast);
                likely_assert(false, "error evaluating: %s", statement->data);
                likely_release_mat(statement);
            }
            likely_release_env(env);
            likely_release_ast(ast);
        }
        likely_release_mat(code);
    }

    likely_assert(assert_.getValue().empty(), "unreached assertion: %s", assert_.getValue().data());
    likely_release_env(parent);
    likely_shutdown();
    return EXIT_SUCCESS;
}
//! [console_interpreter_compiler implementation.]
