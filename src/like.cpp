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
#include <likely.h>

// Until Microsoft implements snprintf
#if _MSC_VER
#define snprintf _snprintf
#endif

using namespace llvm;
using namespace std;

static cl::opt<string> input(cl::Positional, cl::desc("<input file or string>"), cl::init(""));
static cl::opt<string> output(cl::Positional, cl::desc("<output file>"), cl::init(""));
static cl::opt<string> record("record", cl::desc("%d-formatted file to render matrix output to"));
static cl::opt<string> assert_("assert", cl::desc("Confirm the output equals the specified value"));
static cl::opt<bool> ast("ast", cl::desc("Print abstract syntax tree"));
static cl::opt<bool> md5("md5", cl::desc("Print matrix output MD5 hash to terminal"));
static cl::opt<bool> show("show", cl::desc("Show matrix output in a window"));
static cl::opt<bool> quiet("quiet", cl::desc("Don't show matrix output"));
static cl::opt<bool> parallel("parallel" , cl::desc("Compile parallel kernels"));

static void checkOrPrintAndRelease(likely_const_mat input)
{
    const string assertValue = assert_;
    if (assertValue.empty())
        printf("%s\n", input->data);
    else
        likely_assert(!strcmp(input->data, assertValue.c_str()), "expected: %s\n"
                                                          "            got: %s", assertValue.c_str(), input->data);
    likely_release(input);
}

static void replRecord(likely_const_env env, void *context)
{
    if (!context) return;
    static int index = 0;
    const int bufferSize = 128;
    char fileName[bufferSize];
    snprintf(fileName, bufferSize, record.getValue().c_str(), index++);
    likely_mat rendered = likely_render(env->result, NULL, NULL);
    likely_write(rendered, fileName);
    likely_release(rendered);
}

static void replShow(likely_const_env env, void *context)
{
    if (!context) return;
    const string assertValue = assert_;
    if (assertValue.empty()) {
        likely_show_callback(env, context);
    } else {
        likely_mat rendered = likely_render(env->result, NULL, NULL);
        likely_mat baseline = likely_read(assertValue.c_str(), likely_file_binary);
        likely_assert(rendered->channels == baseline->channels, "expected: %d channels, got: %d", baseline->channels, rendered->channels);
        likely_assert(rendered->columns  == baseline->columns , "expected: %d columns, got: %d" , baseline->columns , rendered->columns);
        likely_assert(rendered->rows     == baseline->rows    , "expected: %d rows, got: %d"    , baseline->rows    , rendered->rows);
        likely_assert(rendered->frames   == baseline->frames  , "expected: %d frames, got: %d"  , baseline->frames  , rendered->frames);
        const likely_size elements = rendered->channels * rendered->columns * rendered->rows * rendered->frames;
        likely_size delta = 0;
        for (likely_size i=0; i<elements; i++)
            delta += abs(int(rendered->data[i]) - int(baseline->data[i]));
        likely_release(rendered);
        likely_release(baseline);
        likely_assert(delta < 2*elements /* arbitrary threshold */, "average delta: %g", float(delta) / float(elements));
    }
}

static void replMD5(likely_const_env env, void *context)
{
    if (!context) return;
    likely_mat md5 = likely_md5(env->result);
    checkOrPrintAndRelease(likely_to_hex(md5));
    likely_release(md5);
}

static void replQuiet(likely_const_env, void *)
{
    return;
}

static void replPrint(likely_const_env env, void *context)
{
    if (!context) return;
    checkOrPrintAndRelease(likely_print(env->result));
}

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    likely_repl_callback repl_callback;
    if (!record.getValue().empty()) repl_callback = replRecord;
    else if (show)  repl_callback = replShow;
    else if (md5)   repl_callback = replMD5;
    else if (quiet) repl_callback = replQuiet;
    else            repl_callback = replPrint;

    likely_env parent;
    if (output.empty()) parent = likely_new_env_jit(); // Interpreter
    else                parent = likely_new_env_offline(output.c_str()); // Static compiler
    if (parallel)
        parent->type |= likely_environment_parallel;

    if (input.empty()) {
        // REPL shell
        cout << "Likely\n";
        while (true) {
            cout << "> ";
            string line;
            getline(cin, line);
            likely_ast ast = likely_ast_from_string(line.c_str(), false);
            likely_env env = likely_eval(ast->atoms[0], parent);
            likely_release_ast(ast);
            if (env->type & likely_environment_erratum) {
                likely_release_env(env);
            } else {
                likely_release_env(parent);
                parent = env;
                repl_callback(env, (void*) true);
            }
        }
    } else {
        likely_mat code = likely_read(input.c_str(), likely_file_text);
        bool gfm;
        if (code) {
            gfm = (input.size() >= 3) &&
                  (input.substr(input.size()-3) != ".lk");
        } else {
            gfm = false;
            code = likely_string(input.c_str());
        }

        if (ast) {
            likely_ast parsed = likely_ast_from_string(code->data, gfm);
            for (size_t i=0; i<parsed->num_atoms; i++)
                checkOrPrintAndRelease(likely_ast_to_string(parsed->atoms[i]));
            likely_release_ast(parsed);
        } else {
            likely_ast ast = likely_ast_from_string(code->data, gfm);
            likely_release_env(likely_repl(ast, parent, repl_callback, NULL));
            likely_release_ast(ast);
        }
        likely_release(code);
    }

    likely_release_env(parent);
    likely_shutdown();
    return EXIT_SUCCESS;
}
