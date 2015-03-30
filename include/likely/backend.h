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
 * \brief Compilation options.
 * \see \ref likely_jit
 */
struct likely_settings
{
    int opt_level; /*!< \brief Specify the basic optimization level. 0 = -O0, 1 = -O1, 2 = -O2, 3 = -O3. */
    int size_level; /*!< \brief How much we're optimizing for size. 0 = none, 1 = -Os, 2 = -Oz. */
    bool multicore; /*!< \brief Enable parallel code generation. */
    bool heterogeneous; /*!< \brief Enable heterogeneous code generation. */
    bool unroll_loops; /*!< \brief Perform loop unrolling optimization. */
    bool vectorize_loops; /*!< \brief Perform loop vectorization optimization. */
    bool verbose; /*!< \brief Verbose compiler output. */
};

/*!
 * \brief Default just-in-time compiler options.
 * \par Implementation
 * \snippet src/backend.cpp likely_jit implementation.
 * \param[in] verbose Verbose compiler output.
 * \return \ref likely_settings initialized for just-in-time compilation.
 */
LIKELY_EXPORT struct likely_settings likely_jit(bool verbose);

typedef struct likely_environment *likely_env; /*!< \brief Pointer to a \ref likely_environment. */
typedef struct likely_environment const *likely_const_env; /*!< \brief Pointer to a constant \ref likely_environment. */

/*!
 * \brief The context in which the semantics of an abstract syntax tree is determined.
 * \par Environment Construction
 * | Function                       | Description                          |
 * |--------------------------------|--------------------------------------|
 * | \ref likely_standard           | \copybrief likely_standard           |
 * | \ref likely_eval               | \copybrief likely_eval               |
 * | \ref likely_lex_parse_and_eval | \copybrief likely_lex_parse_and_eval |
 *
 * \see \ref reference_counting
 */
struct likely_environment
{
    likely_const_env parent; /*!< \brief Additional context, or \c NULL if root. */
    likely_const_ast ast; /*!< \brief Associated source code. */
    struct likely_settings *settings; /*!< \brief Used internally to control compiler behavior. */
    struct likely_module *module; /*!< \brief Used internally as a container to store generated instructions during static compilation. */
    struct likely_expression const *expr; /*!< The result of interpreting \ref ast in the context of \ref parent. \c NULL if an error occured. */
    uint32_t ref_count; /*!< \brief Reference count used by \ref likely_retain_env and \ref likely_release_env to track ownership. */
};

/*!
 * \brief Construct a compilation environment with \ref likely_standard_library symbols.
 *
 * Code is written to \p output when the returned \ref likely_environment is deleted by \ref likely_release_env.
 * If \p output is \c NULL, just-in-time compilation will be performed instead.
 *
 * \param[in] settings Compiler options.
 * \param[in] output Where to save the compilation output for static compilation, or \c NULL for just-in-time compilation.
 * \param[in] file_type Format of \p output, ignored if \p output is \c NULL. Valid options are \ref likely_file_ir, \ref likely_file_bitcode, \ref likely_file_object or \ref likely_file_assembly.
 * \return A new compilation environment.
 * \remark This function is \ref thread-unsafe.
 */
LIKELY_EXPORT likely_env likely_standard(struct likely_settings settings, likely_mat *output, likely_file_type file_type);

/*!
 * \brief Re-construct a compiled function from bitcode.
 *
 * For use with \ref likely_function.
 * \param[in] bitcode Output from \ref likely_standard.
 * \param[in] symbol Function name.
 * \return A pre-compiled compilation environment.
 * \remark This function is \ref thread-safe.
 */
LIKELY_EXPORT likely_env likely_precompiled(likely_const_mat bitcode, const char *symbol);

/*!
 * \brief Retain a reference to an environment.
 *
 * Increments \ref likely_environment::ref_count.
 * \param[in] env Environment to add a reference. May be \c NULL.
 * \return \p env.
 * \remark This function is \ref reentrant.
 * \see \ref likely_release_env
 */
LIKELY_EXPORT likely_env likely_retain_env(likely_const_env env);

/*!
 * \brief Release a reference to an environment.
 *
 * Decrements \ref likely_environment::ref_count.
 * \param[in] env Environment to subtract a reference. May be \c NULL.
 * \remark This function is \ref reentrant.
 * \see \ref likely_retain_env
 */
LIKELY_EXPORT void likely_release_env(likely_const_env env);

/*!
 * \brief Obtain the result of a computation as a \ref likely_matrix.
 * \param[in] expr Expression holding the computation.
 * \return The result of the computation as a \ref likely_matrix. \ref owned_by \p expr.
 * \remark This function is \ref reentrant.
 * \see \ref likely_function
 */
LIKELY_EXPORT likely_const_mat likely_result(const struct likely_expression *expr);

/*!
 * \brief Obtain the function pointer of a compilation.
 * \param[in] expr Expression holding the compilation.
 * \return Pointer to the compiled function, or \c NULL if not possible. \ref owned_by \p expr.
 * \remark This function is \ref thread-safe.
 * \see \ref likely_result
 */
LIKELY_EXPORT void *likely_function(const struct likely_expression *expr);

/*!
 * \brief Signature of a function to call after a statement is completed.
 * \see \ref likely_eval
 */
typedef void (*likely_eval_callback)(likely_const_env env, void *context);

/*!
 * \brief Evaluate a series of statements.
 *
 * The input to this function is usually the output from \ref likely_parse.
 * This function will modify \p ast->type to change atom values to their correct type.
 * \param[in] ast Statements to evaluate.
 * \param[in] parent Environment in which to evaluate \p ast.
 * \param[in] eval_callback Function to call with the output of each completed statement.
 * \param[in] context User-defined data to pass to \p eval_callback.
 * \return A new \ref likely_environment holding the final evaluation result.
 * \remark This function is \ref reentrant.
 * \see \ref likely_lex_parse_and_eval
 */
LIKELY_EXPORT likely_env likely_eval(likely_ast ast, likely_const_env parent, likely_eval_callback eval_callback, void *context);

/*!
 * \brief Convenient alternative to \ref likely_lex_and_parse followed by \ref likely_eval.
 * \par Implementation
 * \snippet src/backend.cpp likely_lex_parse_and_eval implementation.
 * \param[in] source Code from which to extract tokens and build the abstract syntax tree.
 * \param[in] file_type How to interpret \p source when extracting tokens.
 * \param[in] parent Environment in which to evaluate \p source.
 * \return A new \ref likely_environment holding the final evaluation result.
 * \remark This function is \ref reentrant.
 */
LIKELY_EXPORT likely_env likely_lex_parse_and_eval(const char *source, likely_file_type file_type, likely_const_env parent);

/*!
 * \brief Contents of the Likely Standard Library: <tt>library/standard.tex</tt>.
 */
LIKELY_EXPORT extern const char likely_standard_library[];

/*!
 * \brief Used internally to hold data structures required for dynamic dispatch.
 * \see \ref likely_dynamic
 */
typedef struct likely_virtual_table *likely_vtable;

/*!
 * \brief Used internally for dynamic dispatching.
 *
 * Since dynamic dispatching does code generation based on runtime argument types,
 * it follows that parameter types must be inspectable at runtime.
 * In other words, dynamic parameters should be specified using \ref likely_mat.
 *
 * \note This function is used internally and should not be called directly.
 * \param[in] vtable Virtual function table for retrieving or compiling the appropriate function based on the types of \p mats.
 * \param[in] mats Array of arguments to pass to the function with dynamic types. The length of \p mats is known by \p vtable.
 * \param[in] data Array of arguments to pass to the function with static types. The layout of \p data is known by \p vtable.
 * \return The result of calling the dynamically-dispatched function.
 * \remark This function is \ref reentrant.
 */
LIKELY_EXPORT likely_mat likely_dynamic(likely_vtable vtable, likely_const_mat *mats, const void *data);

/*!
 * \brief Deallocate objects created to perform compilation.
 *
 * Call _once_ after the calling application finishes using functionality provided in \ref backend.
 * \remark This function is \ref thread-unsafe.
 */
LIKELY_EXPORT void likely_shutdown();

/** @} */ // end of backend

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIKELY_BACKEND_H
