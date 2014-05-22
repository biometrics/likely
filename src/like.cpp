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

static cl::opt<string> input (cl::Positional, cl::desc("<input file>" ), cl::init(""));
static cl::opt<string> output(cl::Positional, cl::desc("<output file>"), cl::init(""));
static cl::opt<string> record("record", cl::desc("%d-formatted file to render matrix output to"));
static cl::opt<bool> source("source", cl::desc("Treat input as a source code string instead of a file"));
static cl::opt<bool> gui("gui", cl::desc("Show matrix output in a window"));
static cl::opt<bool> quiet("quiet", cl::desc("Don't show matrix output"));

static void quietShowCallback(likely_const_mat, likely_const_ast, void *)
{
    return;
}

static void recordShowCallback(likely_const_mat m, likely_const_ast, void *)
{
    static int index = 0;
    const int bufferSize = 128;
    char fileName[bufferSize];
    snprintf(fileName, bufferSize, record.getValue().c_str(), index++);
    likely_mat rendered = likely_render(m, NULL, NULL);
    likely_write(rendered, fileName);
    likely_release(rendered);
}

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    if (!record.getValue().empty()) likely_set_show_callback(recordShowCallback, NULL);
    else if (gui); // No configuration needed, this is the default
    else if (quiet) likely_set_show_callback(quietShowCallback, NULL);
    else            likely_set_show_callback(NULL, NULL); // Print to terminal

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
        bool gfm;
        likely_mat code;
        if (source) {
            gfm = false;
            code = likely_string(input.c_str());
        } else {
            gfm = (input.getValue().substr(input.getValue().size()-3) != ".lk");
            code = likely_read(input.c_str(), likely_file_text);
            likely_assert(code != NULL, "failed to read input file");
        }

        likely_env env;
        if (output.empty()) env = likely_new_env_jit(); // Interpreter
        else                env = likely_new_env_offline(output.c_str(), true); // Static compiler
        likely_release_env(likely_repl((const char*)code->data, gfm, env, NULL));
        likely_release_env(env);
        likely_release(code);
    }

    return EXIT_SUCCESS;
}
