#ifndef LIKELY_H
#define LIKELY_H

#include <likely/backend.h>
#include <likely/export.h>
#include <likely/frontend.h>
#include <likely/io.h>
#include <likely/runtime.h>

/*!
 * \mainpage Likely API
 *
 * Welcome to the Likely API documentation!
 * Both the \c likely and \c dream exectuables are written on top of this interface, so anything they can do you can do too!
 *
 * \section getting_started Getting Started
 * Likely has a \c C API.
 * Your project should <b><tt>\#include \<likely.h\></tt></b> and link against <b><tt>likely</tt></b>.
 * Note that all Likely symbols are prefixed with \c likely_ and follow a \c lowercase_underscore naming convention.
 * <i>[CMake](http://www.cmake.org/)</i> users may appreciate \c LikelyConfig.cmake provided in \c share/likely/.
 *
 * Start reading \ref hello_world_jit.
 *
 * \section reference_counting Reference Counting
 * All [data structures](annotated.html) in the Likely API are passed by pointer-to-reference-counted structs.
 * This is done for several reasons, including:
 * - Cheap shallow copying and ownership transfering.
 * - Optimized structs with dynamically allocated data.
 * - Hidden implementation details.
 * - Consistency of API.
 *
 * As a result, end users should make use of the provided functions for _retaining_ (shallow copying), and _releasing_ (deleting) objects:
 *
 * | Structure                        | Reference                                | Retain / Release                                  |
 * |----------------------------------|------------------------------------------|---------------------------------------------------|
 * | \ref likely_matrix               | \ref likely_mat \n \ref likely_const_mat | \ref likely_retain_mat \n \ref likely_release_mat |
 * | \ref likely_abstract_syntax_tree | \ref likely_ast \n \ref likely_const_ast | \ref likely_retain_ast \n \ref likely_release_ast |
 * | \ref likely_error                | \ref likely_err \n \ref likely_const_err | \ref likely_retain_err \n \ref likely_release_err |
 * | \ref likely_environment          | \ref likely_env \n \ref likely_const_env | \ref likely_retain_env \n \ref likely_release_env |
 * | \ref likely_function             | \ref likely_fun \n \ref likely_const_fun | \ref likely_retain_fun \n \ref likely_release_fun |
 *
 * \note All functions that return a reference to one of these structures assume that the user takes ownership of the reference and should release it when no longer needed.
 *
 * The reference count is kept in a variable named \c ref_count which is incremented and decremented by calls to \a retain and \a release respectively.
 * When the reference count is decremented to zero, any memory associated with the structure will be freed.
 * End users should _not_ modify the \c ref_count variable directly.
 */

#endif // LIKELY_H
