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

#include <likely/runtime.h>
#include <likely/frontend.h>

struct likely_expression;
struct likely_resources;

typedef likely_size likely_environment_type; /* Offline : 1
                                                Erratum : 1
                                                Definition : 1 */

enum likely_environment_type_field
{
    likely_environment_void       = 0x00000000,
    likely_environment_offline    = 0x00000001,
    likely_environment_erratum    = 0x00000002,
    likely_environment_definition = 0x00000004
};

struct likely_environment
{
    struct likely_environment const *parent;
    const char *name;
    union {
        const struct likely_expression *value; // definition
        likely_const_mat result;               // !definition
    };
    struct likely_resources *resources;
    size_t ref_count;
    likely_environment_type type;

#ifdef __cplusplus
private:
    likely_environment();
    likely_environment(struct likely_environment const &);
    likely_environment &operator=(struct likely_environment const &);
#endif // __cplusplus
};

typedef struct likely_environment *likely_env;
typedef struct likely_environment const *likely_const_env;

typedef void *likely_function;
typedef likely_mat (*likely_function_0)();
typedef likely_mat (*likely_function_1)(likely_const_mat);
typedef likely_mat (*likely_function_2)(likely_const_mat, likely_const_mat);
typedef likely_mat (*likely_function_3)(likely_const_mat, likely_const_mat, likely_const_mat);
typedef likely_mat (*likely_function_n)(likely_const_mat*);

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Environments
LIKELY_EXPORT likely_env likely_new_env(likely_const_env parent);
LIKELY_EXPORT likely_env likely_new_env_jit();
LIKELY_EXPORT likely_env likely_new_env_offline(const char *file_name, bool native);
LIKELY_EXPORT likely_env likely_retain_env(likely_const_env env);
LIKELY_EXPORT void likely_release_env(likely_const_env env);

LIKELY_EXPORT bool likely_offline(likely_environment_type type);
LIKELY_EXPORT void likely_set_offline(likely_environment_type *type, bool offline);
LIKELY_EXPORT bool likely_erratum(likely_environment_type type);
LIKELY_EXPORT void likely_set_erratum(likely_environment_type *type, bool error);
LIKELY_EXPORT bool likely_definition(likely_environment_type type);
LIKELY_EXPORT void likely_set_definition(likely_environment_type *type, bool definition);

// Compilation
LIKELY_EXPORT likely_function likely_compile(likely_const_ast ast, likely_env env, likely_type type, ...);
LIKELY_EXPORT likely_function likely_retain_function(likely_function function);
LIKELY_EXPORT void likely_release_function(likely_function function);

// Evaluation
LIKELY_EXPORT likely_env likely_eval(likely_const_ast ast, likely_const_env parent);
LIKELY_EXPORT likely_env likely_repl(const char *source, bool GFM, likely_const_env parent, likely_const_env prev);

// Contents of library/standard.l
LIKELY_EXPORT extern const char likely_standard_library[];

// Dynamic dispatch
typedef struct likely_virtual_table *likely_vtable;
LIKELY_EXPORT likely_mat likely_dynamic(likely_vtable vtable, likely_const_mat *m);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIKELY_BACKEND_H
