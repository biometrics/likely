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

/*!
 * \defgroup backend Backend
 * \brief Compile an abstract syntax tree into a function (\c likely/backend.h).
 * @{
 */

/*!
 * \brief How to interpret \ref likely_environment.
 *
 * Available options are listed in \ref likely_environment_type_mask.
 */
typedef uint32_t likely_environment_type;

/*!
 * \brief \ref likely_environment_type bit format.
 */
enum likely_environment_type_mask
{
    likely_environment_void          = 0x00000000, /*!< \brief An invalid environment. */
    likely_environment_offline       = 0x00000001, /*!< \brief Static compilation. */
    likely_environment_parallel      = 0x00000002, /*!< \brief Generate parallel code. */
    likely_environment_heterogeneous = 0x00000004, /*!< \brief Generate heterogeneous code. */
    likely_environment_erratum       = 0x00000008, /*!< \brief Error indicator. */
    likely_environment_definition    = 0x00000010, /*!< \brief Defines a variable. */
    likely_environment_global        = 0x00000020, /*!< \brief Global scope. */
    likely_environment_abandoned     = 0x00000040, /*!< \brief Does not maintain a reference to \ref likely_environment::parent. */
    likely_environment_base          = 0x00000080, /*!< \brief Owns \ref likely_environment::module. */
};

typedef struct likely_expression *likely_expr; /*!< \brief Pointer to an expression (defined internally). */
typedef struct likely_expression const *likely_const_expr; /*!< \brief Pointer to a constant expression (defined internally). */

typedef struct likely_environment *likely_env; /*!< \brief Pointer to a \ref likely_environment. */
typedef struct likely_environment const *likely_const_env; /*!< \brief Pointer to a constant \ref likely_environment. */

struct likely_environment
{
    likely_size type;
    likely_const_env parent;
    likely_const_ast ast;
    struct likely_module *module;
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
typedef likely_mat (*likely_function_n)(likely_const_mat const*);

typedef struct likely_function *likely_fun;
typedef struct likely_function const *likely_const_fun;

struct likely_function
{
    void *function;
    size_t ref_count;
};

// Environments
LIKELY_EXPORT likely_env likely_new_env(likely_const_env parent);
LIKELY_EXPORT likely_env likely_new_env_jit();
LIKELY_EXPORT likely_env likely_new_env_offline(const char *file_name);
LIKELY_EXPORT likely_env likely_retain_env(likely_const_env env);
LIKELY_EXPORT void likely_release_env(likely_const_env env);

// Compilation
LIKELY_EXPORT likely_fun likely_compile(likely_const_ast ast, likely_const_env env, likely_size type, ...);
LIKELY_EXPORT likely_fun likely_retain_function(likely_const_fun f);
LIKELY_EXPORT void likely_release_function(likely_const_fun f);

// Evaluation
// These functions will modify ast->type to change atom values to their correct type,
// and parent->children / parent->num_children to add the newly constructed environment.
LIKELY_EXPORT likely_env likely_eval(likely_ast ast, likely_env parent);
typedef void (*likely_repl_callback)(likely_const_env env, void *context);
LIKELY_EXPORT likely_env likely_repl(likely_ast ast, likely_env parent, likely_repl_callback repl_callback, void *context);
LIKELY_EXPORT likely_const_env likely_evaluated_expression(likely_const_expr expr);

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
LIKELY_EXPORT void likely_shutdown();

/** @} */ // end of backend

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIKELY_BACKEND_H
