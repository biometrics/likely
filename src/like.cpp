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
#include <likely.h>

static void help()
{
    printf("Usage:\n"
           "  like <likely_expression> <symbol_name> <likely_type> ... <likely_type> <object_file>\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if ((argc < 2) || !strcmp(argv[1], "--help"))
        help();

    likely_ir ir = likely_ir_from_expression(argv[1]);
    likely_arity n = (likely_arity) argc-4;
    likely_type *types = (likely_type*) malloc(n * sizeof(likely_type));
    for (likely_arity i=0; i<n; i++)
        types[i] = likely_type_from_string(argv[i+3]);

    likely_compile_to_file(ir, argv[2], types, n, argv[argc-1], true);
}
