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

#include <cstdlib>
#include <vector>
#include <likely.h>

using namespace std;

// This function should be provided by the likely static compiler
extern "C" likely_mat likely_test_function(const likely_const_mat *args);

int main(int argc, char *argv[])
{
    vector<likely_const_mat> args;
    for (int i=1; i<argc; i+=2) {
        bool ok;
        const likely_type type = likely_type_from_string(argv[i+1], &ok);
        likely_assert(ok, "expected a type, got: %s", argv[i+1]);

        const likely_const_mat arg = likely_read(argv[i], likely_file_media, type);
        likely_assert(arg != NULL, "failed to read: %s", argv[i]);
        args.push_back(arg);
    }
    likely_assert(!args.empty(), "expected at least one argument, the return value.");

    const likely_const_mat result = likely_test_function(args.data() + 1);
    const likely_const_mat rendered = likely_render(result, NULL, NULL);
    likely_assert_approximate(args[0], rendered, 0.03f);
    likely_release_mat(rendered);
    likely_release_mat(result);

    for (likely_const_mat arg : args)
        likely_release_mat(arg);

    return EXIT_SUCCESS;
}
