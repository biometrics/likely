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
#include <likely.h>

using namespace std;

static void execute(const char *source, likely_env env)
{
    likely_const_ast asts = likely_asts_from_string(source);
    if (!asts) return;
    for (size_t i=0; i<asts->num_atoms; i++) {
        likely_const_mat m = likely_eval(asts->atoms[i], env);
        if (!m) continue;
        likely_mat str = likely_to_string(m, true);
        printf("%s\n", (const char*) str->data);
        likely_release(str);
        likely_release(m);
    }
    likely_release_ast(asts);
}

static void error_callback(likely_error error, void *)
{
    cerr << error.what << endl;
}

int main(int argc, char *argv[])
{
    likely_set_error_callback(error_callback, NULL);

    if (argc == 1) {
        // Enter a REPL
        likely_env env = likely_new_env();
        cout << "Likely\n";
        while (true) {
            cout << "> ";
            string line;
            getline(cin, line);
            execute(line.c_str(), env);
        }
        likely_release_env(env);
    } else if (argc == 2) {
        if (!strcmp(argv[1], "--help")) {
            // Print usage
            printf("Usage:\n"
                   "  like\n"
                   "  like <source_file>\n"
                   "  like <likely_expression> <symbol_name> <likely_type> ... <likely_type> <object_file>\n");
        } else {
            // Execute a source file
            ifstream file(argv[1]);
            const string source((istreambuf_iterator<char>(file)),
                                 istreambuf_iterator<char>());
            likely_env env = likely_new_env();
            execute(source.c_str(), env);
            likely_release_env(env);
        }
    } else {
        // Static compiler
        likely_const_ast ast = likely_ast_from_string(argv[1]);
        likely_arity n = (likely_arity) argc-4;
        likely_type *types = (likely_type*) malloc(n * sizeof(likely_type));
        for (likely_arity i=0; i<n; i++)
            types[i] = likely_type_from_string(argv[i+3]);

        likely_env env = likely_new_env();
        likely_compile_to_file(ast, env, argv[2], types, n, argv[argc-1], true);
        likely_release_env(env);
        likely_release_ast(ast);
        free(types);
    }

    return EXIT_SUCCESS;
}
