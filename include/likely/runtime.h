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

// Matrix types
typedef uintptr_t likely_size;
typedef likely_size likely_type; /* Depth : 8
                                    Signed : 1
                                    Floating : 1
                                    Parallel : 1
                                    Heterogeneous : 1
                                    Single-channel : 1
                                    Single-column : 1
                                    Single-row : 1
                                    Single-frame : 1
                                    Saturation : 1 */

enum likely_type_field
{
    likely_matrix_void     = 0x00000000,
    likely_matrix_depth    = 0x000000FF,
    likely_matrix_signed   = 0x00000100,
    likely_matrix_floating = 0x00000200,
    likely_matrix_data     = likely_matrix_depth | likely_matrix_signed | likely_matrix_floating,
    likely_matrix_u1  = 1,
    likely_matrix_u8  = 8,
    likely_matrix_u16 = 16,
    likely_matrix_u32 = 32,
    likely_matrix_u64 = 64,
    likely_matrix_i8  = 8  | likely_matrix_signed,
    likely_matrix_i16 = 16 | likely_matrix_signed,
    likely_matrix_i32 = 32 | likely_matrix_signed,
    likely_matrix_i64 = 64 | likely_matrix_signed,
    likely_matrix_f16 = 16 | likely_matrix_floating | likely_matrix_signed,
    likely_matrix_f32 = 32 | likely_matrix_floating | likely_matrix_signed,
    likely_matrix_f64 = 64 | likely_matrix_floating | likely_matrix_signed,
    likely_matrix_parallel        = 0x00000400,
    likely_matrix_heterogeneous   = 0x00000800,
    likely_matrix_execution       = likely_matrix_parallel | likely_matrix_heterogeneous,
    likely_matrix_multi_channel   = 0x00001000,
    likely_matrix_multi_column    = 0x00002000,
    likely_matrix_multi_row       = 0x00004000,
    likely_matrix_multi_frame     = 0x00008000,
    likely_matrix_multi_dimension = likely_matrix_multi_channel | likely_matrix_multi_column | likely_matrix_multi_row | likely_matrix_multi_frame,
    likely_matrix_saturation      = 0x00010000,
    likely_matrix_magic           = 0xFF000000,
    likely_matrix_matrix          = 0x66000000,
    likely_matrix_native    = sizeof(likely_size)*8,
    likely_matrix_type_type = likely_matrix_native
};

// Disable 'nonstandard extension used : zero-sized array in struct/union' warning
#ifdef _MSC_VER
#  pragma warning(disable: 4200)
#endif // _MSC_VER

// The main datatype in Likely
struct likely_matrix
{
    likely_size bytes, ref_count;
    likely_size channels, columns, rows, frames;
    likely_type type;
    char data[];
};
typedef struct likely_matrix const *likely_const_mat;
typedef struct likely_matrix *likely_mat;

// Abort-style error handling
LIKELY_EXPORT void likely_assert(bool condition, const char *format, ...);

// Type manipulation helper functions
LIKELY_EXPORT size_t likely_get(size_t type, size_t mask);
LIKELY_EXPORT void likely_set(size_t *type, size_t value, size_t mask);
LIKELY_EXPORT bool likely_get_bool(size_t type, size_t mask);
LIKELY_EXPORT void likely_set_bool(size_t *type, bool value, size_t mask);

// Query and edit the matrix type
LIKELY_EXPORT size_t likely_depth(likely_type type);
LIKELY_EXPORT void likely_set_depth(likely_type *type, size_t depth);
LIKELY_EXPORT bool likely_signed(likely_type type);
LIKELY_EXPORT void likely_set_signed(likely_type *type, bool signed_);
LIKELY_EXPORT bool likely_floating(likely_type type);
LIKELY_EXPORT void likely_set_floating(likely_type *type, bool floating);
LIKELY_EXPORT likely_type likely_data(likely_type type);
LIKELY_EXPORT void likely_set_data(likely_type *type, likely_type data);
LIKELY_EXPORT bool likely_parallel(likely_type type);
LIKELY_EXPORT void likely_set_parallel(likely_type *type, bool parallel);
LIKELY_EXPORT bool likely_heterogeneous(likely_type type);
LIKELY_EXPORT void likely_set_heterogeneous(likely_type *type, bool heterogeneous);
LIKELY_EXPORT likely_type likely_execution(likely_type type);
LIKELY_EXPORT void likely_set_execution(likely_type *type, likely_type execution);
LIKELY_EXPORT bool likely_multi_channel(likely_type type);
LIKELY_EXPORT void likely_set_multi_channel(likely_type *type, bool multi_channel);
LIKELY_EXPORT bool likely_multi_column(likely_type type);
LIKELY_EXPORT void likely_set_multi_column(likely_type *type, bool multi_column);
LIKELY_EXPORT bool likely_multi_row(likely_type type);
LIKELY_EXPORT void likely_set_multi_row(likely_type *type, bool multi_row);
LIKELY_EXPORT bool likely_multi_frame(likely_type type);
LIKELY_EXPORT void likely_set_multi_frame(likely_type *type, bool multi_frame);
LIKELY_EXPORT likely_type likely_multi_dimension(likely_type type);
LIKELY_EXPORT void likely_set_multi_dimension(likely_type *type, likely_type multi_dimension);
LIKELY_EXPORT bool likely_saturation(likely_type type);
LIKELY_EXPORT void likely_set_saturation(likely_type *type, bool saturation);
LIKELY_EXPORT size_t likely_magic(likely_type type);
LIKELY_EXPORT void likely_set_magic(likely_type *type, size_t magic);

// Matrix size
LIKELY_EXPORT likely_size likely_elements(likely_const_mat m);
LIKELY_EXPORT likely_size likely_bytes(likely_const_mat m);

// Matrix creation
LIKELY_EXPORT likely_mat likely_new(likely_type type, likely_size channels, likely_size columns, likely_size rows, likely_size frames, void const *data);
LIKELY_EXPORT likely_mat likely_scalar(likely_type type, double value);
LIKELY_EXPORT likely_mat likely_scalar_n(likely_type type, double *values, size_t n);
LIKELY_EXPORT likely_mat likely_scalar_va(likely_type type, double value, ...);
LIKELY_EXPORT likely_mat likely_string(const char *str);
LIKELY_EXPORT likely_mat likely_void();
LIKELY_EXPORT likely_mat likely_copy(likely_const_mat m);
LIKELY_EXPORT likely_mat likely_retain(likely_const_mat m);
LIKELY_EXPORT void likely_release(likely_const_mat m);

// Element access
LIKELY_EXPORT double likely_element(likely_const_mat m, likely_size c, likely_size x, likely_size y, likely_size t);
LIKELY_EXPORT void likely_set_element(likely_mat m, double value, likely_size c, likely_size x, likely_size y, likely_size t);

// Type conversion
LIKELY_EXPORT likely_mat likely_type_to_string(likely_type type);
LIKELY_EXPORT likely_mat likely_type_field_to_string(likely_type type);
LIKELY_EXPORT likely_type likely_type_from_string(const char *str);
LIKELY_EXPORT likely_type likely_type_field_from_string(const char *str, bool *ok);
LIKELY_EXPORT likely_type likely_type_from_value(double value);
LIKELY_EXPORT likely_type likely_type_from_types(likely_type lhs, likely_type rhs);

// Parallelization
// In contrast to likely_dynamic, thunk parameters are known at compile time
// and may therefore take an arbitrary internally-defined structure.
typedef void (*likely_thunk)(void *args, likely_size start, likely_size stop);
LIKELY_EXPORT void likely_fork(likely_thunk thunk, void *args, likely_size size);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIKELY_RUNTIME_H
