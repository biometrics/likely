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

typedef struct likely_environment *likely_env; /*!< \brief Pointer to a \ref likely_environment. */
typedef struct likely_environment const *likely_const_env; /*!< \brief Pointer to a constant \ref likely_environment. */

/*!
 * \brief The context in which the semantics of an abstract syntax tree is determined.
 */
struct likely_environment
{
    likely_const_env parent; /*!< \brief Additional context, or \c NULL if root. */
    likely_const_ast ast; /*!< \brief Associated source code. */
    struct likely_module *module; /*!< \brief Used internally as a container to store the generated instructions. */
    union {
        struct likely_expression const *value; /*!< \brief If <tt>\ref type & \ref likely_environment_definition</tt>, the environment is a \a definition and \ref value is its right-hand-side. */
        likely_const_mat result; /*!< \brief If not <tt>\ref type & \ref likely_environment_definition</tt>, the environment is an \a expression and \ref result is its value. */
    }; /*!< \brief A definition or an expression. */
    likely_environment_type type; /*!< Interpretation of \ref likely_environment. */
    uint32_t ref_count; /*!< \brief Reference count used by \ref likely_retain_env and \ref likely_release_env to track ownership. */
    uint32_t num_children; /*!< \brief Length of \ref children. */
    likely_const_env *children; /*!< \brief Environments where this is the parent. */
};

typedef struct likely_function *likely_fun; /*!< \brief Pointer to a \ref likely_function. */
typedef struct likely_function const *likely_const_fun; /*!< \brief Pointer to a constant \ref likely_function. */

/*!
 * \brief The output of compilation.
 */
struct likely_function
{
    void *function; /*!< \brief Pointer to the resulting executable function with a \c C ABI. */
    uint32_t ref_count; /*!< \brief Reference count. */
};

/*!
 * \brief Construct a new environment for just-in-time compilation.
 * \return A new just-in-time compilation environment.
 */
LIKELY_EXPORT likely_env likely_new_env_jit();

/*!
 * \brief Construct a new environment for static compilation.
 *
 * The extension of \p file_name dictates the type of output.
 * Recognized file extensions are:
 * - \c ll - LLVM IR (unoptimized)
 * - \c bc - LLVM bit code (unoptimized)
 * - \c s - Assembly (optimized for native machine)
 * - \c o - Object file (optimized for native machine)
 *
 * Code is written to \p file_name when the returned \ref likely_environment is deleted by \ref likely_release_env.
 *
 * \param[in] file_name Where to save the compilation output.
 * \return A new static compilation environment.
 */
LIKELY_EXPORT likely_env likely_new_env_offline(const char *file_name);

/*!
 * \brief Retain a reference to an environment.
 *
 * Increments \ref likely_environment::ref_count.
 * \param[in] env Environment to add a reference. May be \c NULL.
 * \return \p env.
 * \see likely_release_env
 */
LIKELY_EXPORT likely_env likely_retain_env(likely_const_env env);

/*!
 * \brief Release a reference to an environment.
 *
 * Decrements \ref likely_environment::ref_count.
 * \param[in] env Environment to subtract a reference. May be \c NULL.
 * \see likely_retain_env
 */
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
LIKELY_EXPORT likely_const_env likely_evaluated_expression(struct likely_expression const *expr);

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
