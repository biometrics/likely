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

#ifndef __LIKELY_H
#define __LIKELY_H

#include <assert.h>
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

// C API starts here
#ifdef __cplusplus
extern "C" {
#endif

// The documentation and source code for the Likely Standard Library
LIKELY_EXPORT const char *likely_index_html();

// Encodes matrix metadata
typedef uint32_t likely_hash; /* Depth : 8
                                 Signed : 1
                                 Floating : 1
                                 Parallel : 1
                                 Heterogeneous : 1
                                 Single-channel : 1
                                 Single-column : 1
                                 Single-row : 1
                                 Single-frame : 1
                                 Owner : 1
                                 Reserved : 15 */

// Convenience values for editing a likely_hash
enum likely_hash_field
{
    likely_hash_null = 0x00000000,
    likely_hash_depth = 0x000000FF,
    likely_hash_signed = 0x00000100,
    likely_hash_floating = 0x00000200,
    likely_hash_type = likely_hash_depth | likely_hash_signed | likely_hash_floating,
    likely_hash_u8  = 8,
    likely_hash_u16 = 16,
    likely_hash_u32 = 32,
    likely_hash_u64 = 64,
    likely_hash_i8  = 8  | likely_hash_signed,
    likely_hash_i16 = 16 | likely_hash_signed,
    likely_hash_i32 = 32 | likely_hash_signed,
    likely_hash_i64 = 64 | likely_hash_signed,
    likely_hash_f16 = 16 | likely_hash_floating | likely_hash_signed,
    likely_hash_f32 = 32 | likely_hash_floating | likely_hash_signed,
    likely_hash_f64 = 64 | likely_hash_floating | likely_hash_signed,
    likely_hash_parallel = 0x00000400,
    likely_hash_heterogeneous = 0x00000800,
    likely_hash_single_channel = 0x00001000,
    likely_hash_single_column = 0x00002000,
    likely_hash_single_row = 0x00004000,
    likely_hash_single_frame = 0x00008000,
    likely_hash_owner = 0x00010000,
    likely_hash_reserved = 0xFFFE0000
};

// The only struct in the API
struct likely_matrix
{
    uint8_t *data;
    uint32_t channels, columns, rows, frames;
    likely_hash hash;
};

// You shouldn't need to call these directly
inline int likely_get(likely_hash hash, likely_hash_field mask) { return hash & mask; }
inline void likely_set(likely_hash &hash, int i, likely_hash_field mask) { hash &= ~mask; hash |= i & mask; }
inline bool likely_get_bool(likely_hash hash, likely_hash_field mask) { return hash & mask; }
inline void likely_set_bool(likely_hash &hash, bool b, likely_hash_field mask) { b ? hash |= mask : hash &= ~mask; }

// Convenience functions for querying and editing the hash
inline int  likely_depth(likely_hash hash) { return likely_get(hash, likely_hash_depth); }
inline void likely_set_depth(likely_hash &hash, int depth) { likely_set(hash, depth, likely_hash_depth); }
inline bool likely_is_signed(likely_hash hash) { return likely_get_bool(hash, likely_hash_signed); }
inline void likely_set_signed(likely_hash &hash, bool is_signed) { likely_set_bool(hash, is_signed, likely_hash_signed); }
inline bool likely_is_floating(likely_hash hash) { return likely_get_bool(hash, likely_hash_floating); }
inline void likely_set_floating(likely_hash &hash, bool is_floating) { likely_set_bool(hash, is_floating, likely_hash_floating); }
inline int  likely_type(likely_hash hash) { return likely_get(hash, likely_hash_type); }
inline void likely_set_type(likely_hash &hash, int type) { likely_set(hash, type, likely_hash_type); }
inline bool likely_is_parallel(likely_hash hash) { return likely_get_bool(hash, likely_hash_parallel); }
inline void likely_set_parallel(likely_hash &hash, bool is_parallel) { likely_set_bool(hash, is_parallel, likely_hash_parallel); }
inline bool likely_is_heterogeneous(likely_hash hash) { return likely_get_bool(hash, likely_hash_heterogeneous); }
inline void likely_set_heterogeneous(likely_hash &hash, bool is_heterogeneous) { likely_set_bool(hash, is_heterogeneous, likely_hash_heterogeneous); }
inline bool likely_is_single_channel(likely_hash hash) { return likely_get_bool(hash, likely_hash_single_channel); }
inline void likely_set_single_channel(likely_hash &hash, bool is_single_channel) { likely_set_bool(hash, is_single_channel, likely_hash_single_channel); }
inline bool likely_is_single_column(likely_hash hash) { return likely_get_bool(hash, likely_hash_single_column); }
inline void likely_set_single_column(likely_hash &hash, bool is_single_column) { likely_set_bool(hash, is_single_column, likely_hash_single_column); }
inline bool likely_is_single_row(likely_hash hash) { return likely_get_bool(hash, likely_hash_single_row); }
inline void likely_set_single_row(likely_hash &hash, bool is_single_row) { likely_set_bool(hash, is_single_row, likely_hash_single_row); }
inline bool likely_is_single_frame(likely_hash hash) { return likely_get_bool(hash, likely_hash_single_frame); }
inline void likely_set_single_frame(likely_hash &hash, bool is_single_frame) { likely_set_bool(hash, is_single_frame, likely_hash_single_frame); }
inline bool likely_is_owner(likely_hash hash) { return likely_get_bool(hash, likely_hash_owner); }
inline void likely_set_owner(likely_hash &hash, bool is_owner) { likely_set_bool(hash, is_owner, likely_hash_owner); }
inline int  likely_reserved(likely_hash hash) { return likely_get(hash, likely_hash_reserved); }
inline void likely_set_reserved(likely_hash &hash, int reserved) { likely_set(hash, reserved, likely_hash_reserved); }

// Convenience functions for determining matrix size
inline uint32_t likely_elements(const likely_matrix *m) { return m->channels * m->columns * m->rows * m->frames; }
inline uint32_t likely_bytes(const likely_matrix *m) { return uint64_t(likely_depth(m->hash)) * uint64_t(likely_elements(m)) / uint64_t(8); }

// Convenience functions for default initializing a matrix
inline void likely_matrix_initialize(likely_matrix *m, uint8_t *data, uint32_t channels, uint32_t columns, uint32_t rows, uint32_t frames, likely_hash hash)
{
    m->data = data;
    m->channels = channels;
    m->columns = columns;
    m->rows = rows;
    m->frames = frames;
    m->hash = hash;
    likely_set_single_channel(m->hash, channels == 1);
    likely_set_single_column(m->hash, columns == 1);
    likely_set_single_row(m->hash, rows == 1);
    likely_set_single_frame(m->hash, frames == 1);
}
inline void likely_matrix_initialize_null(likely_matrix *m, likely_hash hash = likely_hash_null) { likely_matrix_initialize(m, NULL, 0, 0, 0, 0, hash); }

// Functions for allocating and freeing matrix data
LIKELY_EXPORT void likely_allocate(likely_matrix *m);
LIKELY_EXPORT void likely_free(likely_matrix *m);

// Convenience functions for debugging; by convention c = channel, x = column, y = row, t = frame
LIKELY_EXPORT double likely_element(const likely_matrix *m, uint32_t c = 0, uint32_t x = 0, uint32_t y = 0, uint32_t t = 0);
LIKELY_EXPORT void likely_set_element(likely_matrix *m, double value, uint32_t c = 0, uint32_t x = 0, uint32_t y = 0, uint32_t t = 0);
LIKELY_EXPORT const char *likely_hash_to_string(likely_hash h); // Pointer guaranteed until the next call to this function
LIKELY_EXPORT likely_hash likely_string_to_hash(const char *str);
LIKELY_EXPORT void likely_print_matrix(const likely_matrix *m);
LIKELY_EXPORT void likely_assert(bool condition, const char *format, ...);
LIKELY_EXPORT void likely_dump(); // Print LLVM module contents to stderr

// Helper library functions; you shouldn't call these directly
typedef const char *likely_description;
typedef uint8_t likely_arity;
typedef uint32_t likely_size;
typedef void (*likely_nullary_kernel)(likely_matrix *dst, likely_size start, likely_size stop);
typedef void (*likely_unary_kernel)(const likely_matrix *src, likely_matrix *dst, likely_size start, likely_size stop);
typedef void (*likely_binary_kernel)(const likely_matrix *srcA, const likely_matrix *srcB, likely_matrix *dst, likely_size start, likely_size stop);
typedef void (*likely_ternary_kernel)(const likely_matrix *srcA, const likely_matrix *srcB, const likely_matrix *srcC, likely_matrix *dst, likely_size start, likely_size stop);
LIKELY_EXPORT void *likely_make_function(likely_description description, likely_arity arity);
LIKELY_EXPORT void *likely_make_allocation(likely_description description, likely_arity arity, likely_matrix *src, ...);
LIKELY_EXPORT void *likely_make_kernel(likely_description description, likely_arity arity, likely_matrix *src, ...);
LIKELY_EXPORT void likely_parallel_dispatch(void *kernel, likely_arity arity, likely_size start, likely_size stop, likely_matrix *src, ...);

// Core library functions
typedef void (*likely_nullary_function)(likely_matrix *dst);
inline likely_nullary_function likely_make_nullary_function(likely_description description)
    { return (likely_nullary_function)likely_make_function(description, 0); }

typedef void (*likely_unary_function)(const likely_matrix *src, likely_matrix *dst);
inline likely_unary_function likely_make_unary_function(likely_description description)
    { return (likely_unary_function)likely_make_function(description, 1); }

typedef void (*likely_binary_function)(const likely_matrix *srcA, const likely_matrix *srcB, likely_matrix *dst);
inline likely_binary_function likely_make_binary_function(likely_description description)
    { return (likely_binary_function)likely_make_function(description, 2); }

typedef void (*likely_ternary_function)(const likely_matrix *srcA, const likely_matrix *srcB, const likely_matrix *srcC, likely_matrix *dst);
inline likely_ternary_function likely_make_ternary_function(likely_description description)
    { return (likely_ternary_function)likely_make_function(description, 3); }

#ifdef __cplusplus
}
#endif

// C++ wrapper starts here
#ifdef __cplusplus
#include <ostream>
#include <string>

namespace likely {

inline std::string indexHTML() { return likely_index_html(); }

struct Matrix : public likely_matrix
{
    Matrix(likely_hash hash = likely_hash_null) { likely_matrix_initialize_null(this, hash); }
    Matrix(uint8_t *data, uint32_t channels, uint32_t columns, uint32_t rows, uint32_t frames, likely_hash hash)
    { likely_matrix_initialize(this, data, channels, columns, rows, frames, hash); }
    ~Matrix() { likely_free(this); }

    inline int     depth() const { return likely_depth(hash); }
    inline Matrix &setDepth(int bits) { likely_set_depth(hash, bits); return *this; }
    inline bool    isFloating() const { return likely_is_floating(hash); }
    inline Matrix &setFloating(bool isFloating) { likely_set_floating(hash, isFloating); return *this; }
    inline bool    isSigned() const { return likely_is_signed(hash); }
    inline Matrix &setSigned(bool isSigned) { likely_set_signed(hash, isSigned); return *this; }
    inline int     type() const { return likely_type(hash); }
    inline Matrix &setType(int type) { likely_set_type(hash, type); return *this; }
    inline bool    parallel() const { return likely_is_parallel(hash); }
    inline Matrix &setParallel(bool parallel) { likely_set_parallel(hash, parallel); return *this; }
    inline bool    heterogeneous() const { return likely_is_heterogeneous(hash); }
    inline Matrix &setHeterogeneous(bool heterogeneous) { likely_set_heterogeneous(hash, heterogeneous); return *this; }
    inline bool    isSingleChannel() const { return likely_is_single_channel(hash); }
    inline Matrix &setSingleChannel(bool isSingleChannel) { likely_set_single_channel(hash, isSingleChannel); return *this; }
    inline bool    isSingleColumn() const { return likely_is_single_column(hash); }
    inline Matrix &setSingleColumn(bool isSingleColumn) { likely_set_single_column(hash, isSingleColumn); return *this; }
    inline bool    isSingleRow() const { return likely_is_single_row(hash); }
    inline Matrix &setSingleRow(bool isSingleRow) { likely_set_single_row(hash, isSingleRow); return *this; }
    inline bool    isSingleFrame() const { return likely_is_single_frame(hash); }
    inline Matrix &setSingleFrame(bool isSingleFrame) { likely_set_single_frame(hash, isSingleFrame); return *this; }
    inline bool    isOwner() const { return likely_is_owner(hash); }
    inline Matrix &setOwner(bool isOwner) { likely_set_owner(hash, isOwner); return *this; }
    inline int     reserved() const { return likely_reserved(hash); }
    inline Matrix &setReserved(int reserved) { likely_set_reserved(hash, reserved); return *this; }

    inline uint32_t elements() const { return likely_elements(this); }
    inline uint32_t bytes() const { return likely_bytes(this); }
    inline double element(uint32_t c, uint32_t x, uint32_t y, uint32_t t) const { return likely_element(this, c, x, y, t); }
    inline Matrix &setElement(double value, uint32_t c, uint32_t x, uint32_t y, uint32_t t) { likely_set_element(this, value, c, x, y, t); return *this; }
    inline void print() const { return likely_print_matrix(this); }
};

typedef likely_nullary_function NullaryFunction;
inline NullaryFunction makeNullaryFunction(const std::string &description)
    { return likely_make_nullary_function(description.c_str()); }

typedef likely_unary_function UnaryFunction;
inline UnaryFunction makeUnaryFunction(const std::string &description)
    { return likely_make_unary_function(description.c_str()); }

typedef likely_binary_function BinaryFunction;
inline BinaryFunction makeBinaryFunction(const std::string &description)
    { return likely_make_binary_function(description.c_str()); }

typedef likely_ternary_function TernaryFunction;
inline TernaryFunction makeTernaryFunction(const std::string &description)
    { return likely_make_ternary_function(description.c_str()); }

} // namespace likely

#endif // __cplusplus

#endif // __LIKELY_H
