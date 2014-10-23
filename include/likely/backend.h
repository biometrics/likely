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
 * \par Environment Construction
 * | Function           | Description              |
 * |--------------------|--------------------------|
 * | \ref likely_jit    | \copybrief likely_jit    |
 * | \ref likely_static | \copybrief likely_static |
 * | \ref likely_eval   | \copybrief likely_eval   |
 * | \ref likely_repl   | \copybrief likely_repl   |
 *
 * \see \ref reference_counting
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
 * \par Function Construction
 * | Function            | Documentation             |
 * |---------------------|---------------------------|
 * | \ref likely_compile | \copybrief likely_compile |
 *
 * \see \ref reference_counting
 */
struct likely_function
{
    void *function; /*!< \brief Pointer to the resulting executable function with a \c C ABI. */
    uint32_t ref_count; /*!< \brief Reference count used by \ref likely_retain_fun and \ref likely_release_fun to track ownership. */
};

/*!
 * \brief Construct a new environment for just-in-time compilation.
 * \return A new just-in-time compilation environment.
 * \see \ref likely_static
 */
LIKELY_EXPORT likely_env likely_jit();

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
 * \see \ref likely_jit
 */
LIKELY_EXPORT likely_env likely_static(const char *file_name);

/*!
 * \brief Retain a reference to an environment.
 *
 * Increments \ref likely_environment::ref_count.
 * \param[in] env Environment to add a reference. May be \c NULL.
 * \return \p env.
 * \see \ref likely_release_env
 */
LIKELY_EXPORT likely_env likely_retain_env(likely_const_env env);

/*!
 * \brief Release a reference to an environment.
 *
 * Decrements \ref likely_environment::ref_count.
 * \param[in] env Environment to subtract a reference. May be \c NULL.
 * \see \ref likely_retain_env
 */
LIKELY_EXPORT void likely_release_env(likely_const_env env);

/*!
 * \brief Compile a function.
 *
 * If an incomplete \p type is specified, dynamic dispatch will occur at runtime
 * based on the types of the \ref likely_matrix arguments.
 * See \ref likely_dynamic for details.
 *
 * \param[in] ast Function abstract syntax tree.
 * \param[in] env Function environment.
 * \param[in] type Function type terminated by \ref likely_matrix_void.
 * \return The compiled \ref likely_function.
 */
LIKELY_EXPORT likely_fun likely_compile(likely_const_ast ast, likely_const_env env, likely_matrix_type type, ...);

/*!
 * \brief Obtain the result of a computation.
 *
 * The returned \ref likely_matrix is owned by \p env.
 * \param[in] env Where the computation was performed.
 * \return The result of the computation, or \c NULL if no computation was performed.
 */
LIKELY_EXPORT likely_const_mat likely_result(likely_const_env env);

/*!
 * \brief Retain a reference to a function.
 *
 * Increments \ref likely_function::ref_count.
 * \param[in] fun Function to add a reference. May be \c NULL.
 * \return \p fun.
 * \see \ref likely_release_fun
 */
LIKELY_EXPORT likely_fun likely_retain_fun(likely_const_fun fun);

/*!
 * \brief Release a reference to a function.
 *
 * Decrements \ref likely_function::ref_count.
 * \param[in] fun Function to subtract a reference. May be \c NULL.
 * \see \ref likely_release_fun
 */
LIKELY_EXPORT void likely_release_fun(likely_const_fun fun);

/*!
 * \brief Evaluate an expression in the context of an environment.
 *
 * This function will modify \p ast->type to change atom values to their correct type,
 * and \p parent->children / \p parent->num_children to add the newly constructed environment.
 * \param[in] ast Statement to evaluate.
 * \param[in] parent Environment in which to evaluate \p ast.
 * \return A new \ref likely_environment holding the evaluation result.
 * \see \ref likely_repl
 */
LIKELY_EXPORT likely_env likely_eval(likely_ast ast, likely_env parent);

/*!
 * \brief Signature of a function to call after a statement is completed.
 * \see \ref likely_repl
 */
typedef void (*likely_repl_callback)(likely_const_env env, void *context);

/*!
 * \brief Read-evaluate-print-loop.
 *
 * Evaluate a series of statements.
 * \param[in] ast Statements to evaluate.
 * \param[in] parent Environment in which to evaluate \p ast.
 * \param[in] repl_callback Function to call with the output of each completed statement.
 * \param[in] context User-defined data to pass to \p repl_callback.
 * \return A new \ref likely_environment holding the final evaluation result.
 * \see \ref likely_eval
 */
LIKELY_EXPORT likely_env likely_repl(likely_ast ast, likely_env parent, likely_repl_callback repl_callback, void *context);

/*!
 * \brief Contents of the Likely Standard Library: <tt>library/standard.ll</tt>.
 */
LIKELY_EXPORT extern const char likely_standard_library[];

/*!
 * \brief Used internally to hold data structures required for dynamic dispatch.
 * \see \ref likely_dynamic
 */
typedef struct likely_virtual_table *likely_vtable;

/*!
 * \brief Used internally for dynamic dispatch.
 *
 * Since dynamic dispatch does code generation based on runtime argument types,
 * it follows that dynamic dispatch can only work on functions where all parameter types can be inspected at runtime.
 * In other words, functions that have only likely_mat parameters.
 * This is in contrast to \ref likely_fork where parameters are known at compile time and may therefore take an arbitrary internally-defined structure.
 * \note This function is used internally and should not be called directly.
 * \param[in] vtable Virtual function table for retrieving or compiling the appropriate function based on the types of \p m.
 * \param[in] mats Array of arguments to pass to the function. The length of \p mats is known by \p vtable.
 * \return The result from calling the dynamically dispatch function.
 */
LIKELY_EXPORT likely_mat likely_dynamic(likely_vtable vtable, likely_const_mat *mats);

/*!
 * \brief Compute the MD5 hash of \ref likely_matrix::data.
 * \param[in] mat \ref likely_matrix::data to compute the MD5 hash of.
 * \return A new matrix where containing the 16-byte MD5 hash.
 */
LIKELY_EXPORT likely_mat likely_md5(likely_const_mat mat);

/*!
 * \brief Deallocate objects created to perform compilation.
 *
 * Call _once_ after the program is done using functionality provided in \ref backend.
 */
LIKELY_EXPORT void likely_shutdown();

/** @} */ // end of backend

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIKELY_BACKEND_H
