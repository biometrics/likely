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
 * These functions are implemented in \c src/runtime_*, and are designed to have
 * absolutely minimal dependencies.
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

typedef likely_size likely_type;

/*!
 * \brief Data type of a matrix.
 *
 * How to interpret \ref likely_matrix::data.
 */
enum likely_matrix_type
{
    likely_matrix_void      = 0x00000000, /*!< \brief Unknown type. */
    likely_matrix_depth     = 0x000000FF, /*!< \brief Bits per element. */
    likely_matrix_floating  = 0x00000100, /*!< \brief Elements are floating-point. */
    likely_matrix_array     = 0x00000200, /*!< \brief Interpret as a pointer to an array of matricies (used internally only). */
    likely_matrix_signed    = 0x00000400, /*!< \brief Elements are signed (integers). */
    likely_matrix_saturated = 0x00000800, /*!< \brief Use saturated arithmetic with computations involving these elements. */
    likely_matrix_element   = likely_matrix_depth
                            | likely_matrix_floating
                            | likely_matrix_array
                            | likely_matrix_signed
                            | likely_matrix_saturated, /*!< \brief The portion of \ref likely_matrix_type indicating how to interpret elements. */
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
    likely_matrix_multi_dimension = likely_matrix_multi_channel
                                  | likely_matrix_multi_column
                                  | likely_matrix_multi_row
                                  | likely_matrix_multi_frame, /*!< \brief The portion of \ref likely_matrix_type indicating matrix dimensionality. */
    likely_matrix_string    = likely_matrix_i8 | likely_matrix_multi_channel, /*!< \brief likely_matrix::data is a C-style string. */
    likely_matrix_native    = sizeof(likely_size)*8, /*!< \brief Native integer size. */
};

// Disable 'nonstandard extension used : zero-sized array in struct/union' warning
#ifdef _MSC_VER
#  pragma warning(disable: 4200)
#endif // _MSC_VER

/*!
 * \brief The principal data type for input and output to compiled functions.
 *
 * The last five fields (_channels_, _columns_, _rows_, _frames_, and _type_) are collectively referred to as the matrix _header_. In contrast to most image processing libraries which tend to feature 3-dimensional matrices (channels, columns, rows), Likely includes a fourth dimension, frames, in order to facilitate processing videos or collections of images.
 *
 * \section Element Access
 * By convention, element layout in the data buffer with resepect to decreasing spatial locality is _channel_, _column_, _row_, _frame_. Thus an element at channel _c_, column _x_, row _y_, and frame _t_, can be retrieved like:
 * \code
 * float likely_get_element(likely_matrix m, likely_size c, likely_size x, likely_size y, likely_size t)
 * {
 *     likely_size columnStep = m->channels;
 *     likely_size rowStep = m->channels * columnStep;
 *     likely_size frameStep = m->rows * rowStep;
 *     likely_size index = t*frameStep + y*rowStep + x*columnStep + c;
 *     assert(likely_type(m) == likely_type_f32);
 *     return reinterpret_cast<float*>(m->data)[index];
 * }
 * \endcode
 *
 * Convenience functions **likely_element** and **likely_set_element** are provided for individual element access. These functions are inefficient for iterating over a large numbers of elements due to the repeated index calculations, and their use is suggested only for debugging purposes or when the matrix is known to be small.
 */
struct likely_matrix
{
    uint32_t ref_count; /*!< \brief Reference count. */
    union {
        enum likely_matrix_type type; /*!< \brief Type of \ref data.*/
        uint32_t set_type; /*!< \brief Type of \ref data.*/
    };

    uint32_t channels; /*!< \brief Dimensionality. */
    uint32_t columns;  /*!< \brief Dimensionality. */
    uint32_t rows;     /*!< \brief Dimensionality. */
    uint32_t frames;   /*!< \brief Dimensionality. */

    uint64_t _reserved; /*!< \brief Used to ensure \ref data is 32-byte aligned: <tt>sizeof(likely_matrix) == 32</tt>. */
    char data[]; /*!< \brief Buffer. */
};

typedef struct likely_matrix const *likely_const_mat; /*!< \brief Pointer to a constant \ref likely_matrix. */
typedef struct likely_matrix *likely_mat; /*!< \brief Pointer to a \ref likely_matrix. */

/*!
 * \defgroup error_handling Error Handling
 * \brief Respond to unexpected conditions.
 * @{
 */

/*!
 * \brief Conditional abort-style error handling with an error message.
 * \param condition If \c false, print \a format and abort.
 * \param format <tt>printf</tt>-style error message.
 */
LIKELY_EXPORT void likely_assert(bool condition, const char *format, ...);
/** @} */ // end of error_handling

/*!
 * \defgroup matrix_size Matrix Size
 * \brief Determine the size of a \ref likely_matrix.
 * @{
 */
LIKELY_EXPORT size_t likely_depth(likely_type type);
LIKELY_EXPORT void likely_set_depth(likely_type *type, size_t depth);
LIKELY_EXPORT likely_size likely_elements(likely_const_mat m);
LIKELY_EXPORT likely_size likely_bytes(likely_const_mat m);
/** @} */ // end of matrix_size

/*!
 * \defgroup matrix_creation Matrix Creation
 * \brief Create a \ref likely_matrix.
 * @{
 */
LIKELY_EXPORT likely_mat likely_new(likely_type type, likely_size channels, likely_size columns, likely_size rows, likely_size frames, void const *data);
LIKELY_EXPORT likely_mat likely_scalar(likely_type type, double value);
LIKELY_EXPORT likely_mat likely_scalar_n(likely_type type, double *values, size_t n);
LIKELY_EXPORT likely_mat likely_scalar_va(likely_type type, double value, ...);
LIKELY_EXPORT likely_mat likely_string(const char *str);
LIKELY_EXPORT likely_mat likely_void();
LIKELY_EXPORT likely_mat likely_copy(likely_const_mat m);
LIKELY_EXPORT likely_mat likely_retain(likely_const_mat m);
LIKELY_EXPORT void likely_release(likely_const_mat m);
/** @} */ // end of matrix_creation

/*!
 * \defgroup element_access Element Access
 * \brief Access elements in a \ref likely_matrix.
 * @{
 */
LIKELY_EXPORT likely_type likely_c_type(likely_type type);
LIKELY_EXPORT double likely_element(likely_const_mat m, likely_size c, likely_size x, likely_size y, likely_size t);
LIKELY_EXPORT void likely_set_element(likely_mat m, double value, likely_size c, likely_size x, likely_size y, likely_size t);
LIKELY_EXPORT bool likely_is_string(likely_const_mat m);
/** @} */ // end of element_access

/*!
 * \defgroup parallelization Parallelization
 * \brief Distribute work in parallel.
 *
 * These functions are used internally and should not be called directly.
 * @{
 */
// In contrast to likely_dynamic, thunk parameters are known at compile time
// and may therefore take an arbitrary internally-defined structure.
typedef void (*likely_thunk)(void *args, likely_size start, likely_size stop);
LIKELY_EXPORT void likely_fork(likely_thunk thunk, void *args, likely_size size);
/** @} */ // end of parallelization

/** @} */ // end of runtime

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIKELY_RUNTIME_H
