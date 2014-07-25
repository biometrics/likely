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

static cl::opt<string> input (cl::Positional, cl::desc("<input file or string>" ), cl::init(""));
static cl::opt<string> output(cl::Positional, cl::desc("<output file>"), cl::init(""));
static cl::opt<string> record("record", cl::desc("%d-formatted file to render matrix output to"));
static cl::opt<string> assert_("assert", cl::desc("Confirm the output equals the specified value"));
static cl::opt<bool> ast("ast", cl::desc("Print abstract syntax tree"));
static cl::opt<bool> md5("md5", cl::desc("Print matrix output MD5 hash to terminal"));
static cl::opt<bool> show("show", cl::desc("Show matrix output in a window"));
static cl::opt<bool> quiet("quiet", cl::desc("Don't show matrix output"));

static void check(likely_const_mat input)
{
    const string assertValue = assert_;
    likely_assert(assertValue.empty() || !strcmp(input->data, assertValue.c_str()), "expected value: %s", assertValue.c_str());
}

static void replRecord(likely_const_mat m, likely_const_ast, void *)
{
    static int index = 0;
    const int bufferSize = 128;
    char fileName[bufferSize];
    snprintf(fileName, bufferSize, record.getValue().c_str(), index++);
    likely_mat rendered = likely_render(m, NULL, NULL);
    likely_write(rendered, fileName);
    likely_release(rendered);
}

static void replMD5(likely_const_mat m, likely_const_ast, void *)
{
    likely_mat md5 = likely_md5(m);
    likely_mat hex = likely_to_hex(md5);
    likely_release(md5);
    printf("%s\n", hex->data);
    check(hex);
    likely_release(hex);
}

static void replQuiet(likely_const_mat, likely_const_ast, void *)
{
    return;
}

static void replPrint(likely_const_mat m, likely_const_ast, void *)
{
    likely_mat str = likely_to_string(m, true);
    printf("%s\n", str->data);
    check(str);
    likely_release(str);
}

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    if (!record.getValue().empty()) likely_set_repl_callback(replRecord, NULL);
    else if (show)  likely_set_repl_callback(likely_show, NULL);
    else if (md5)   likely_set_repl_callback(replMD5, NULL);
    else if (quiet) likely_set_repl_callback(replQuiet, NULL);
    else            likely_set_repl_callback(replPrint, NULL);

    if (input.empty()) {
        // REPL shell
        cout << "Likely\n";
        while (true) {
            cout << "> ";
            string line;
            getline(cin, line);
            likely_repl(line.c_str(), false, NULL, NULL);
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
            for (size_t i=0; i<parsed->num_atoms; i++) {
                likely_mat printed = likely_ast_to_string(parsed->atoms[i]);
                printf("%s\n", printed->data);
                likely_release(printed);
            }
            likely_release_ast(parsed);
        } else {
            likely_env env;
            if (output.empty()) env = likely_new_env_jit(); // Interpreter
            else                env = likely_new_env_offline(output.c_str(), true); // Static compiler
            likely_release_env(likely_repl(code->data, gfm, env, NULL));
            likely_release_env(env);
            likely_release(code);
        }
    }

    return EXIT_SUCCESS;
}
