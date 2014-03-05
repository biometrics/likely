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

#ifndef LIKELY_BACKEND_H
#define LIKELY_BACKEND_H

#include <likely/likely_runtime.h>
#include <likely/likely_frontend.h>

#ifdef __cplusplus
extern "C" {
#endif

// Environments
typedef struct likely_environment *likely_env;
typedef struct likely_environment const *likely_const_env;
LIKELY_EXPORT likely_env likely_new_env();
LIKELY_EXPORT likely_env likely_retain_env(likely_const_env env);
LIKELY_EXPORT void likely_release_env(likely_const_env env);

// Compilation
typedef likely_mat (*likely_function)(likely_const_mat, ...);
typedef likely_mat (*likely_function_n)(likely_const_mat*);
LIKELY_EXPORT likely_function likely_compile(likely_const_ast ast, likely_env env);
LIKELY_EXPORT likely_function_n likely_compile_n(likely_const_ast ast, likely_env env);
LIKELY_EXPORT void likely_compile_to_file(likely_const_ast ast, likely_env env, const char *symbol_name, likely_type *types, likely_arity n, const char *file_name, bool native);
LIKELY_EXPORT void *likely_retain_function(void *function);
LIKELY_EXPORT void likely_release_function(void *function);

// Evaluation
LIKELY_EXPORT likely_mat likely_eval(likely_const_ast ast, likely_env env);

// Contents of library/standard.like
LIKELY_EXPORT extern const char likely_standard_library[];

#ifdef __cplusplus
}
#endif

#endif // LIKELY_BACKEND_H
