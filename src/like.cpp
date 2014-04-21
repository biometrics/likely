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

using namespace llvm;
using namespace std;

static cl::opt<string> input (cl::Positional, cl::desc("<input file>" ), cl::init(""));
static cl::opt<string> output(cl::Positional, cl::desc("<output file>"), cl::init(""));
static cl::opt<bool> gfm("gfm", cl::desc("Tokenize source using Github Flavored Markdown"));
static cl::opt<bool> gui("gui", cl::desc("Show matrix output in a window"));

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    if (!gui)
        likely_set_show_callback(NULL, NULL); // Print to terminal

    if (input.empty()) {
        // REPL shell
        cout << "Likely\n";
        while (true) {
            cout << "> ";
            string line;
            getline(cin, line);
            likely_repl(line.c_str(), gfm, NULL, NULL);
        }
    } else {
        ifstream file(input.c_str());
        const string source((istreambuf_iterator<char>(file)),
                             istreambuf_iterator<char>());
        likely_assert(!source.empty(), "failed to read input file");

        likely_env env;
        if (output.empty()) env = likely_new_env(); // Interpreter
        else                env = likely_new_env_offline(output.c_str(), true); // Static compiler
        likely_repl(source.c_str(), gfm, &env, NULL);
        likely_release_env(env);
    }

    return EXIT_SUCCESS;
}
