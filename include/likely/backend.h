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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef likely_size likely_environment_type; /* Offline    : 1
                                                Erratum    : 1
                                                Definition : 1
                                                Local      : 1 */

enum likely_environment_type_field
{
    likely_environment_void       = 0x00000000,
    likely_environment_offline    = 0x00000001,
    likely_environment_erratum    = 0x00000002,
    likely_environment_definition = 0x00000004,
    likely_environment_global     = 0x00000008
};

struct likely_expression;
typedef struct likely_expression *likely_expr;
typedef struct likely_expression const *likely_const_expr;

struct likely_environment;
typedef struct likely_environment *likely_env;
typedef struct likely_environment const *likely_const_env;

struct likely_resources;
typedef struct likely_resources *likely_res;
typedef struct likely_resources const *likely_const_res;

struct likely_environment
{
    likely_environment_type type;
    likely_const_env parent;
    likely_const_ast ast;
    likely_res resources;
    union {
        likely_const_expr value; // definition
        likely_const_mat result; // !definition
    };
    size_t ref_count, num_children;
    likely_const_env *children;
};

typedef likely_mat (*likely_function_0)();
typedef likely_mat (*likely_function_1)(likely_const_mat);
typedef likely_mat (*likely_function_2)(likely_const_mat, likely_const_mat);
typedef likely_mat (*likely_function_3)(likely_const_mat, likely_const_mat, likely_const_mat);
typedef likely_mat (*likely_function_n)(likely_const_mat*);

struct likely_function
{
    void *function;
    size_t ref_count;
};

typedef struct likely_function *likely_fun;
typedef struct likely_function const *likely_const_fun;

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
LIKELY_EXPORT bool likely_global(likely_environment_type type);
LIKELY_EXPORT void likely_set_global(likely_environment_type *type, bool global);

// Compilation
LIKELY_EXPORT likely_fun likely_compile(likely_const_ast ast, likely_const_env env, likely_type type, ...);
LIKELY_EXPORT likely_fun likely_retain_function(likely_const_fun f);
LIKELY_EXPORT void likely_release_function(likely_const_fun f);

// Evaluation
LIKELY_EXPORT likely_env likely_eval(likely_const_ast ast, likely_const_env parent, likely_const_env previous);
typedef void (*likely_repl_callback)(likely_const_env env, void *context);
LIKELY_EXPORT void likely_set_repl_callback(likely_repl_callback callback, void *context);
LIKELY_EXPORT likely_env likely_repl(const char *source, bool GFM, likely_const_env parent, likely_const_env previous);
LIKELY_EXPORT likely_const_mat likely_evaluated_expression(likely_const_expr expr);

// Contents of library/standard.l
LIKELY_EXPORT extern const char likely_standard_library[];

// Dynamic dispatch
//   Since dynamic dispatch does code generation based on runtime argument types,
//   it follows that dynamic dispatch can only work on functions where all parameter types can be inspected at runtime.
//   In other words, functions that have only likely_mat parameters.
//   This is in contrast to likely_fork where parameters are known at compile time
//   and may therefore take an arbitrary internally-defined structure.
typedef struct likely_virtual_table *likely_vtable;
LIKELY_EXPORT likely_mat likely_dynamic(likely_vtable vtable, likely_const_mat *m);

// Miscellaneous
LIKELY_EXPORT likely_mat likely_md5(likely_const_mat buffer);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIKELY_BACKEND_H
