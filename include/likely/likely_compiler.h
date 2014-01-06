/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2013 Joshua C. Klontz                                           *
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

#ifndef LIKELY_COMPILER_H
#define LIKELY_COMPILER_H

#include <likely/likely_runtime.h>

#ifdef __cplusplus
extern "C" {
#endif

struct lua_State;
typedef struct lua_State* likely_ir;
LIKELY_EXPORT likely_ir likely_ir_from_string(const char *str);
LIKELY_EXPORT const char *likely_ir_to_string(likely_ir ir);

typedef uint8_t likely_arity;
typedef likely_mat (*likely_function)(likely_const_mat, ...);
typedef likely_mat (*likely_function_n)(likely_const_mat*);
LIKELY_EXPORT likely_function likely_compile(likely_ir ir); // Takes ownership of ir
LIKELY_EXPORT likely_function_n likely_compile_n(likely_ir ir); // Takes ownership of ir
LIKELY_EXPORT void likely_compile_to_file(likely_ir ir, const char *symbol_name, likely_type *types, likely_arity n, const char *file_name, bool native); // Does _not_ take ownership of ir

LIKELY_EXPORT void likely_stack_dump(struct lua_State *L, int levels);

#ifdef __cplusplus
}
#endif

#endif // LIKELY_COMPILER_H
