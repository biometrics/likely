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

#ifndef LIKELY_H
#define LIKELY_H

#include <stddef.h>
#include <stdint.h>

// Don't worry about this
#if defined LIKELY_LIBRARY
#  if defined _WIN32 || defined __CYGWIN__
#    define LIKELY_EXPORT __declspec(dllexport)
#  else
#    define LIKELY_EXPORT __attribute__((visibility("default")))
#  endif
#else
#  if defined _WIN32 || defined __CYGWIN__
#    define LIKELY_EXPORT __declspec(dllimport)
#  else
#    define LIKELY_EXPORT
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Contents of standard.likely
LIKELY_EXPORT const char *likely_standard_library();

// Matrix types
typedef uint8_t likely_data;
typedef uintptr_t likely_size;
typedef uint32_t likely_type; /* Depth : 8
                                 Signed : 1
                                 Floating : 1
                                 Parallel : 1
                                 Heterogeneous : 1
                                 Single-channel : 1
                                 Single-column : 1
                                 Single-row : 1
                                 Single-frame : 1
                                 Saturation : 1
                                 Reserved : 15 */

// Standard type masks and values
enum likely_type_field
{
    likely_type_null     = 0x00000000,
    likely_type_depth    = 0x000000FF,
    likely_type_signed   = 0x00000100,
    likely_type_floating = 0x00000200,
    likely_type_mask     = likely_type_depth | likely_type_signed | likely_type_floating,
    likely_type_u8  = 8,
    likely_type_u16 = 16,
    likely_type_u32 = 32,
    likely_type_u64 = 64,
    likely_type_i8  = 8  | likely_type_signed,
    likely_type_i16 = 16 | likely_type_signed,
    likely_type_i32 = 32 | likely_type_signed,
    likely_type_i64 = 64 | likely_type_signed,
    likely_type_f16 = 16 | likely_type_floating | likely_type_signed,
    likely_type_f32 = 32 | likely_type_floating | likely_type_signed,
    likely_type_f64 = 64 | likely_type_floating | likely_type_signed,
    likely_type_parallel        = 0x00000400,
    likely_type_heterogeneous   = 0x00000800,
    likely_type_multi_channel   = 0x00001000,
    likely_type_multi_column    = 0x00002000,
    likely_type_multi_row       = 0x00004000,
    likely_type_multi_frame     = 0x00008000,
    likely_type_multi_dimension = 0x0000F000,
    likely_type_saturation      = 0x00010000,
    likely_type_reserved        = 0xFFFE0000
};

// The only struct in the API
typedef struct
{
    likely_data *data;
    struct likely_matrix_private *d_ptr; // Private data for internal bookkeeping
    likely_size channels, columns, rows, frames;
    likely_type type;
} likely_matrix;
typedef likely_matrix *likely_mat;
typedef const likely_matrix *likely_const_mat;

// Query and edit the type
LIKELY_EXPORT int  likely_depth(likely_type type);
LIKELY_EXPORT void likely_set_depth(likely_type *type, int depth);
LIKELY_EXPORT bool likely_signed(likely_type type);
LIKELY_EXPORT void likely_set_signed(likely_type *type, bool signed_);
LIKELY_EXPORT bool likely_floating(likely_type type);
LIKELY_EXPORT void likely_set_floating(likely_type *type, bool floating);
LIKELY_EXPORT bool likely_parallel(likely_type type);
LIKELY_EXPORT void likely_set_parallel(likely_type *type, bool parallel);
LIKELY_EXPORT bool likely_heterogeneous(likely_type type);
LIKELY_EXPORT void likely_set_heterogeneous(likely_type *type, bool heterogeneous);
LIKELY_EXPORT bool likely_multi_channel(likely_type type);
LIKELY_EXPORT void likely_set_multi_channel(likely_type *type, bool multi_channel);
LIKELY_EXPORT bool likely_multi_column(likely_type type);
LIKELY_EXPORT void likely_set_multi_column(likely_type *type, bool multi_column);
LIKELY_EXPORT bool likely_multi_row(likely_type type);
LIKELY_EXPORT void likely_set_multi_row(likely_type *type, bool multi_row);
LIKELY_EXPORT bool likely_multi_frame(likely_type type);
LIKELY_EXPORT void likely_set_multi_frame(likely_type *type, bool multi_frame);
LIKELY_EXPORT bool likely_saturation(likely_type type);
LIKELY_EXPORT void likely_set_saturation(likely_type *type, bool saturation);
LIKELY_EXPORT int  likely_reserved(likely_type type);
LIKELY_EXPORT void likely_set_reserved(likely_type *type, int reserved);

// Determine matrix size
LIKELY_EXPORT likely_size likely_elements(likely_const_mat m);
LIKELY_EXPORT likely_size likely_bytes(likely_const_mat m);

// Create and manage a reference counted matrix
LIKELY_EXPORT likely_mat likely_new(likely_type type = likely_type_f32, likely_size channels = 1, likely_size columns = 1, likely_size rows = 1, likely_size frames = 1, likely_data *data = NULL, int8_t copy = 0);
LIKELY_EXPORT likely_mat likely_scalar(double value);
LIKELY_EXPORT likely_mat likely_copy(likely_const_mat m, int8_t copy_data = 0);
LIKELY_EXPORT likely_mat likely_retain(likely_mat m);
LIKELY_EXPORT void likely_release(likely_mat m);

// Matrix I/O
LIKELY_EXPORT likely_mat likely_read(const char *file_name);
LIKELY_EXPORT void likely_write(likely_const_mat image, const char *file_name);
LIKELY_EXPORT likely_mat likely_decode(likely_const_mat buffer);
LIKELY_EXPORT likely_mat likely_encode(likely_const_mat image, const char *extension);
LIKELY_EXPORT likely_mat likely_render(likely_const_mat m, double *min = NULL, double *max = NULL); // Return a 888 matrix for visualization

// Debugging functionality
LIKELY_EXPORT double likely_element(likely_const_mat m, likely_size c = 0, likely_size x = 0, likely_size y = 0, likely_size t = 0);
LIKELY_EXPORT void likely_set_element(likely_mat m, double value, likely_size c = 0, likely_size x = 0, likely_size y = 0, likely_size t = 0);
LIKELY_EXPORT const char *likely_type_to_string(likely_type type); // Return value managed internally and guaranteed until the next call to this function
LIKELY_EXPORT likely_type likely_type_from_string(const char *str);
LIKELY_EXPORT likely_type likely_type_from_value(double value);
LIKELY_EXPORT likely_type likely_type_from_types(likely_type lhs, likely_type rhs);
LIKELY_EXPORT void likely_print(likely_const_mat m);

// Core library types and functions
typedef const char *likely_description;
typedef uint8_t likely_arity;

typedef likely_mat (*likely_function_0)(void);
typedef likely_mat (*likely_function_1)(likely_const_mat);
typedef likely_mat (*likely_function_2)(likely_const_mat, likely_const_mat);
typedef likely_mat (*likely_function_3)(likely_const_mat, likely_const_mat, likely_const_mat);
LIKELY_EXPORT void *likely_compile(likely_description description, likely_arity n, likely_type type, ...);
LIKELY_EXPORT void *likely_compile_n(likely_description description, likely_arity n, likely_type *types);

// Used internally for OpenMP-like parallelization
LIKELY_EXPORT void _likely_fork(void *thunk, likely_arity arity, likely_size size, likely_const_mat src, ...);

// Make Likely accessible as a Lua module
struct lua_State;
LIKELY_EXPORT int luaopen_likely(lua_State *L);
LIKELY_EXPORT lua_State *likely_exec(const char *source, lua_State *L = NULL);
LIKELY_EXPORT void likely_stack_dump(lua_State *L, int levels = 1);

#ifdef __cplusplus
}
#endif

#endif // LIKELY_H
