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

static void execute(const char *source, likely_env env)
{
    likely_const_ast asts = likely_asts_from_string(source, true);
    if (!asts) return;
    for (size_t i=0; i<asts->num_atoms; i++) {
        likely_const_mat m = likely_eval(asts->atoms[i], env);
        if (!m) continue;
        likely_mat str = likely_to_string(m, true);
        printf("%s\n", str->data);
        likely_release(str);
        likely_release(m);
    }
    likely_release_ast(asts);
}

int main(int argc, char *argv[])
{
    cl::ParseCommandLineOptions(argc, argv);

    if (input.empty()) {
        // REPL shell
        likely_env env = likely_new_jit();
        cout << "Likely\n";
        while (true) {
            cout << "> ";
            string line;
            getline(cin, line);
            execute(line.c_str(), env);
        }
        likely_release_env(env);
    } else {
        ifstream file(input.c_str());
        const string source((istreambuf_iterator<char>(file)),
                             istreambuf_iterator<char>());
        likely_assert(!source.empty(), "failed to read input file");

        likely_env env;
        if (output.empty()) env = likely_new_jit(); // Interpreter
        else                env = likely_new_offline(output.c_str(), true); // Static compiler
        execute(source.c_str(), env);
        likely_release_env(env);
    }

    return EXIT_SUCCESS;
}
