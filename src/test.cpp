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
extern "C" likely_mat likely_test_function(likely_const_mat *args);

int main(int argc, char *argv[])
{
    vector<likely_const_mat> args;
    for (int i=1; i<argc; i++) {
        const likely_const_mat arg = likely_read(argv[i], likely_file_media, likely_image);
        likely_assert(arg != NULL, "failed to read: %s", argv[i]);
        args.push_back(arg);
    }

    for (likely_const_mat arg : args)
        likely_release_mat(arg);

    return EXIT_SUCCESS;
}
