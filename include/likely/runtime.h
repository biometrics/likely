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

#ifndef LIKELY_RUNTIME_H
#define LIKELY_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <likely/export.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*!
 * \defgroup runtime Runtime
 * \brief Symbols in \c likely/runtime.h.
 *
 * Statically compiled Likely algorithms will generally depend on these symbols
 * <i>and these symbols only</i>.
 *
 * Unless otherwise noted, these functions are implemented in \c src/runtime_common.c and designed to have no dependencies outside of the \c C Standard Library.
 * Use these symbols by linking against the \c likely_runtime static library,
 * the complete \c likely library, or by compiling the relevant source files
 * directly into your project.
 *
 * @{
 */

/*!
 * \brief Size type guaranteed to be native width.
 */
typedef uintptr_t likely_size;

/*!
 * \brief \ref likely_matrix data type.
 *
 * Indicates how to interpret \ref likely_matrix::data.
 * \see likely_matrix_type_mask
 */
typedef uint32_t likely_matrix_type;

/*!
 * \brief \ref likely_matrix_type bit format.
 */
enum likely_matrix_type_mask
{
    likely_matrix_void      = 0x00000000, /*!< \brief Unknown type. */
    likely_matrix_depth     = 0x000000FF, /*!< \brief Bits per element. */
    likely_matrix_floating  = 0x00000100, /*!< \brief Elements are floating-point. */
    likely_matrix_array     = 0x00000200, /*!< \brief Interpret as a pointer to an array of matricies (used internally only). */
    likely_matrix_signed    = 0x00000400, /*!< \brief Elements are signed (integers). */
    likely_matrix_c_type    = likely_matrix_depth | likely_matrix_floating | likely_matrix_array | likely_matrix_signed, /*!< \brief The portion of the \ref likely_matrix_type representable in \c C. */
    likely_matrix_saturated = 0x00000800, /*!< \brief Use saturated arithmetic with computations involving these elements. */
    likely_matrix_element   = likely_matrix_c_type | likely_matrix_saturated, /*!< \brief The portion of \ref likely_matrix_type indicating how to interpret elements. */
    likely_matrix_u1  = 1, /*!< \brief 1-bit unsigned integer elements. */
    likely_matrix_u8  = 8, /*!< \brief 8-bit unsigned integer elements. */
    likely_matrix_u16 = 16, /*!< \brief 16-bit unsigned integer elements. */
    likely_matrix_u32 = 32, /*!< \brief 32-bit unsigned integer elements. */
    likely_matrix_u64 = 64, /*!< \brief 64-bit unsigned integer elements. */
    likely_matrix_i8  = 8  | likely_matrix_signed, /*!< \brief 8-bit signed integer elements. */
    likely_matrix_i16 = 16 | likely_matrix_signed, /*!< \brief 16-bit signed integer elements. */
    likely_matrix_i32 = 32 | likely_matrix_signed, /*!< \brief 32-bit signed integer elements. */
    likely_matrix_i64 = 64 | likely_matrix_signed, /*!< \brief 64-bit signed integer elements. */
    likely_matrix_f16 = 16 | likely_matrix_floating, /*!< \brief 16-bit floating-point elements. */
    likely_matrix_f32 = 32 | likely_matrix_floating, /*!< \brief 32-bit floating-point elements. */
    likely_matrix_f64 = 64 | likely_matrix_floating, /*!< \brief 64-bit floating-point elements. */
    likely_matrix_multi_channel   = 0x00001000, /*!< \brief \ref likely_matrix::channels > 1. */
    likely_matrix_multi_column    = 0x00002000, /*!< \brief \ref likely_matrix::columns > 1. */
    likely_matrix_multi_row       = 0x00004000, /*!< \brief \ref likely_matrix::rows > 1. */
    likely_matrix_multi_frame     = 0x00008000, /*!< \brief \ref likely_matrix::frames > 1. */
    likely_matrix_multi_dimension = likely_matrix_multi_channel | likely_matrix_multi_column | likely_matrix_multi_row | likely_matrix_multi_frame, /*!< \brief The portion of \ref likely_matrix_type indicating matrix dimensionality. Used for loop optimizations. */
    likely_matrix_string = likely_matrix_i8 | likely_matrix_multi_channel, /*!< \brief likely_matrix::data is a C-style string. */
    likely_matrix_native = sizeof(likely_size)*8, /*!< \brief Native integer size. */
};

// Disable 'nonstandard extension used : zero-sized array in struct/union' warning
#ifdef _MSC_VER
#  pragma warning(disable: 4200)
#endif // _MSC_VER

/*!
 * \brief The principal data structure in Likely.
 *
 * The fields excluding \ref data are collectively referred to as the matrix _header_.
 * In contrast to most image processing libraries which tend to feature 3-dimensional matrices (_channels_, _columns_ and _rows_), Likely includes a fourth dimension, _frames_, in order to facilitate processing videos and image collections.
 *
 * \section memory_management
 * Matricies are designed for heap allocation using \ref likely_new, and are passed around by pointer using \ref likely_mat and \ref likely_const_mat.
 * A matrix keeps track of its reference count in \ref likely_matrix::ref_count.
 * References are incremented and decremented using \ref likely_retain and \ref likely_release respectively.
 * A matrix is automatically freed by \ref likely_release when its reference count goes to zero.
 *
 * \section element_access Element Access
 * By convention, element layout in \ref data with respect to decreasing spatial locality is: channel, column, row, frame.
 * Thus an element at channel _c_, column _x_, row _y_, and frame _t_, can be retrieved like:
 *
 * \snippet src/runtime_common.c likely_element implementation.
 *
 * Convenience functions \ref likely_element and \ref likely_set_element are provided for individual element access.
 * These functions should be used sparingly as they are inefficient for iterating over a large numbers of elements due to the repeated index calculations.
 */
struct likely_matrix
{
    uint32_t ref_count; /*!< \brief Reference count.
                         *
                         * Used by \ref likely_retain and \ref likely_release to track ownership.
                         */
    likely_matrix_type type; /*!< \brief Interpretation of \ref data. */
    uint32_t channels; /*!< \brief Sub-spatial dimensionality. */
    uint32_t columns;  /*!< \brief Horizontal dimensionality. */
    uint32_t rows;     /*!< \brief Vertical dimensionality. */
    uint32_t frames;   /*!< \brief Super-spatial (temporal) dimensionality. */
    char data[]; /*!< \brief Buffer. */
};

typedef struct likely_matrix const *likely_const_mat; /*!< \brief Pointer to a constant \ref likely_matrix. */
typedef struct likely_matrix *likely_mat; /*!< \brief Pointer to a \ref likely_matrix. */

/*!
 * \brief Conditional abort-style error handling with an error message.
 * \param[in] condition If \c false, print \a format and abort.
 * \param[in] format <tt>printf</tt>-style error message.
 */
LIKELY_EXPORT void likely_assert(bool condition, const char *format, ...);

/*!
 * \brief The size of \ref likely_matrix::data in bytes.
 *
 * \par Implementation
 * \snippet src/runtime_common.c likely_bytes implementation.
 * \param[in] mat The matrix from which to calculate the data buffer size.
 */
LIKELY_EXPORT size_t likely_bytes(likely_const_mat mat);

/*!
 * \brief Returns \c true if \ref likely_matrix::data is a string.
 *
 * \par Implementation
 * \snippet src/runtime_common.c likely_is_string implementation.
 * \param[in] m The matrix to test.
 * \see likely_string
 */
LIKELY_EXPORT bool likely_is_string(likely_const_mat m);

/*!
 * \brief Allocate, initialize, and return a pointer to a new \ref likely_matrix.
 *
 * In the case that \p data is NULL then the returned \ref likely_matrix::data is uninitialized.
 * Otherwise, the returned \ref likely_matrix::data is initialized by copying the contents of \p data.
 * In the latter case, \p data should be at least size \ref likely_bytes.
 *
 * The \ref likely_matrix_multi_dimension component of \ref likely_matrix::type is set automatically for \p channels, \p columns, \p rows and \p frames greater than one.
 *
 * Release the returned matrix with \ref likely_release.
 *
 * \par Implementation
 * \snippet src/runtime_common.c likely_new implementation.
 * \param[in] type \ref likely_matrix::type.
 * \param[in] channels \ref likely_matrix::channels.
 * \param[in] columns \ref likely_matrix::columns.
 * \param[in] rows \ref likely_matrix::rows.
 * \param[in] frames \ref likely_matrix::frames.
 * \param[in] data \ref likely_matrix::data.
 * \see likely_scalar_n likely_string
 */
LIKELY_EXPORT likely_mat likely_new(likely_matrix_type type, uint32_t channels, uint32_t columns, uint32_t rows, uint32_t frames, void const *data);

/*!
 * \brief Construct a new single-element \ref likely_matrix.
 *
 * Convenient alternative to \ref likely_scalar_n.
 * \param[in] type \ref likely_matrix::type.
 * \param[in] value Element value.
 */
LIKELY_EXPORT likely_mat likely_scalar(likely_matrix_type type, double value);

/*!
 * \brief Construct a new multi-element \ref likely_matrix.
 *
 * Convenient alternative to \ref likely_new.
 * \par Implementation
 * \snippet src/runtime_common.c likely_scalar_n implementation.
 * \param[in] type \ref likely_matrix::type.
 * \param[in] values Array of element values.
 * \param[in] n Length of \p values.
 * \see likely_scalar likely_scalar_va
 */
LIKELY_EXPORT likely_mat likely_scalar_n(likely_matrix_type type, double *values, size_t n);

/*!
 * \brief Construct a new multi-element \ref likely_matrix.
 *
 * Convenient alternative to \ref likely_scalar_n.
 * \param[in] type \ref likely_matrix::type.
 * \param[in] value <tt>NaN</tt>-terminated list of element values.
 */
LIKELY_EXPORT likely_mat likely_scalar_va(likely_matrix_type type, double value, ...);

/*!
 * \brief Construct a new \ref likely_matrix from a string.
 *
 * Convenient alternative to \ref likely_new.
 * \par Implementation
 * \snippet src/runtime_common.c likely_string implementation.
 * \param[in] str String used to initialized \ref likely_matrix::data.
 * \see likely_is_string
 */
LIKELY_EXPORT likely_mat likely_string(const char *str);

/*!
 * \brief Retain a reference to a matrix.
 *
 * Increments \ref likely_matrix::ref_count.
 * \par Implementation
 * \snippet src/runtime_common.c likely_retain implementation.
 * \param[in] mat Matrix to add an additional reference. May be NULL.
 * \see likely_release
 */
LIKELY_EXPORT likely_mat likely_retain(likely_const_mat mat);

/*!
 * \brief Release a reference to a matrix.
 *
 * Decrements \ref likely_matrix::ref_count.
 * Frees the matrix memory when the reference count is decremented to zero.
 * \par Implementation
 * \snippet src/runtime_common.c likely_release implementation.
 * \param[in] mat Matrix to add an additional reference. May be NULL.
 * \see likely_retain
 */
LIKELY_EXPORT void likely_release(likely_const_mat mat);

/*!
 * \brief Return a matrix value at the specified location.
 *
 * A \c NULL \p m or out-of-bounds \p c, \p x, \p y or \p t will return \p NAN.
 * \ref likely_assert is called if the matrix does not have a type convertible to \c C.
 * \par Implementation
 * \snippet src/runtime_common.c likely_element implementation.
 * \param[in] m The matrix to index into.
 * \param[in] c Channel.
 * \param[in] x Column.
 * \param[in] y Row.
 * \param[in] t Frame.
 * \see likely_set_element
 */
LIKELY_EXPORT double likely_element(likely_const_mat m, uint32_t c, uint32_t x, uint32_t y, uint32_t t);

/*!
 * \brief Set a matrix value at the specified location.
 *
 * A \c NULL \p m or out-of-bounds \p c, \p x, \p y or \p t will return without setting \p value.
 * \ref likely_assert is called if the matrix does not have a type convertible to \c C.
 * \param[in] m The matrix to index into.
 * \param[in] value The value to set the element to.
 * \param[in] c Channel.
 * \param[in] x Column.
 * \param[in] y Row.
 * \param[in] t Frame.
 * \see likely_element
 */
LIKELY_EXPORT void likely_set_element(likely_mat m, double value, uint32_t c, uint32_t x, uint32_t y, uint32_t t);

/*!
 * \brief A special kind of function designed to be run in parallel.
 * \see likely_fork
 */
typedef void (*likely_thunk)(void *args, size_t start, size_t stop);

/*!
 * \brief Execute work in parallel.
 *
 * In contrast to likely_dynamic, thunk parameters are known at compile time and may therefore take an arbitrary internally-defined structure.
 * The implementation is very similar to how \a OpenMP works.
 * [Here](https://software.intel.com/en-us/blogs/2010/07/23/thunk-you-very-much-or-how-do-openmp-compilers-work-part-2) is a good introductory article on the subject.
 *
 * This function is implemented in \c src/runtime_stdthread.cpp and depends on the presence of a <tt>C++11</tt>-compatible standard library.
 * \note This function is used internally and should not be called directly.
 * \param[in] thunk The function to run.
 * \param[in] args The arguments to propogate to \p thunk.
 * \param[in] size The range [0, \p size) over which to execute \p thunk.
 */
LIKELY_EXPORT void likely_fork(likely_thunk thunk, void *args, size_t size);

/** @} */ // end of runtime

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIKELY_RUNTIME_H
