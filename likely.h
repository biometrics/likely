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

// The documentation and source code for Likely
LIKELY_EXPORT const char *likely_index_html();

// The only struct in the API
struct likely_matrix
{
    uint8_t *data;
    uint32_t channels, columns, rows, frames;
    uint16_t hash; /* Depth : 8
                      Signed : 1
                      Floating : 1
                      Parallel : 1
                      Heterogeneous : 1
                      Single-channel : 1
                      Single-column : 1
                      Single-row : 1
                      Single-frame : 1 */

    enum Hash { Depth = 0x00FF,
                Signed = 0x0100,
                Floating = 0x0200,
                Type = Depth | Signed | Floating,
                Parallel = 0x0400,
                Heterogeneous = 0x0800,
                SingleChannel = 0x1000,
                SingleColumn = 0x2000,
                SingleRow = 0x4000,
                SingleFrame = 0x8000,
                u1  = 1,
                u8  = 8,
                u16 = 16,
                u32 = 32,
                u64 = 64,
                i8  = 8  | Signed,
                i16 = 16 | Signed,
                i32 = 32 | Signed,
                i64 = 64 | Signed,
                f16 = 16 | Floating | Signed,
                f32 = 32 | Floating | Signed,
                f64 = 64 | Floating | Signed };
};

// You shouldn't need to call these directly
inline int likely_get(const likely_matrix *m, likely_matrix::Hash h) { return m->hash & h; }
inline void likely_set(likely_matrix *m, int i, likely_matrix::Hash h) { m->hash &= ~h; m->hash |= i & h; }
inline bool likely_get_bool(const likely_matrix *m, likely_matrix::Hash h) { return m->hash & h; }
inline void likely_set_bool(likely_matrix *m, bool b, likely_matrix::Hash h) { b ? m->hash |= h : m->hash &= ~h; }

// Convenience functions for querying and editing the hash
inline int  likely_depth(const likely_matrix *m) { return likely_get(m, likely_matrix::Depth); }
inline void likely_set_depth(likely_matrix *m, int bits) { likely_set(m, bits, likely_matrix::Depth); }
inline bool likely_is_signed(const likely_matrix *m) { return likely_get_bool(m, likely_matrix::Signed); }
inline void likely_set_signed(likely_matrix *m, bool is_signed) { likely_set_bool(m, is_signed, likely_matrix::Signed); }
inline bool likely_is_floating(const likely_matrix *m) { return likely_get_bool(m, likely_matrix::Floating); }
inline void likely_set_floating(likely_matrix *m, bool is_floating) { likely_set_bool(m, is_floating, likely_matrix::Floating); }
inline int  likely_type(const likely_matrix *m) { return likely_get(m, likely_matrix::Type); }
inline void likely_set_type(likely_matrix *m, int type) { likely_set(m, type, likely_matrix::Type); }
inline bool likely_is_parallel(const likely_matrix *m) { return likely_get_bool(m, likely_matrix::Parallel); }
inline void likely_set_parallel(likely_matrix *m, bool is_parallel) { likely_set_bool(m, is_parallel, likely_matrix::Parallel); }
inline bool likely_is_heterogeneous(const likely_matrix *m) { return likely_get_bool(m, likely_matrix::Heterogeneous); }
inline void likely_set_heterogeneous(likely_matrix *m, bool is_heterogeneous) { likely_set_bool(m, is_heterogeneous, likely_matrix::Heterogeneous); }
inline bool likely_is_single_channel(const likely_matrix *m) { return likely_get_bool(m, likely_matrix::SingleChannel); }
inline void likely_set_single_channel(likely_matrix *m, bool is_single_channel) { likely_set_bool(m, is_single_channel, likely_matrix::SingleChannel); }
inline bool likely_is_single_column(const likely_matrix *m) { return likely_get_bool(m, likely_matrix::SingleColumn); }
inline void likely_set_single_column(likely_matrix *m, bool is_single_column) { likely_set_bool(m, is_single_column, likely_matrix::SingleColumn); }
inline bool likely_is_single_row(const likely_matrix *m) { return likely_get_bool(m, likely_matrix::SingleRow); }
inline void likely_set_single_row(likely_matrix *m, bool is_single_row) { likely_set_bool(m, is_single_row, likely_matrix::SingleRow); }
inline bool likely_is_single_frame(const likely_matrix *m) { return likely_get_bool(m, likely_matrix::SingleFrame); }
inline void likely_set_single_frame(likely_matrix *m, bool is_single_frame) { likely_set_bool(m, is_single_frame, likely_matrix::SingleFrame); }

// Convenience functions for determining matrix size
inline uint32_t likely_elements(const likely_matrix *m) { return m->channels * m->columns * m->rows * m->frames; }
inline uint32_t likely_bytes(const likely_matrix *m) { return likely_depth(m) / 8 * likely_elements(m); }

// Convenience functions for default initializing a matrix
inline void likely_matrix_initialize(likely_matrix *m, uint32_t channels, uint32_t columns, uint32_t rows, uint32_t frames, uint16_t hash)
{
    m->data = NULL;
    m->channels = channels;
    m->columns = columns;
    m->rows = rows;
    m->frames = frames;
    m->hash = hash;
    likely_set_single_channel(m, channels == 1);
    likely_set_single_column(m, columns == 1);
    likely_set_single_row(m, rows == 1);
    likely_set_single_frame(m, frames == 1);
}
inline void likely_matrix_initialize_null(likely_matrix *m) { likely_matrix_initialize(m, 0, 0, 0, 0, 0); }

// Convenience functions for element access
// By convention c = channel, x = column, y = row, t = frame
LIKELY_EXPORT double likely_element(const likely_matrix *m, uint32_t c = 0, uint32_t x = 0, uint32_t y = 0, uint32_t t = 0);
LIKELY_EXPORT void likely_set_element(likely_matrix *m, double value, uint32_t c = 0, uint32_t x = 0, uint32_t y = 0, uint32_t t = 0);

// Core library functions
LIKELY_EXPORT void *likely_make_function(const char *description, uint8_t arity); // You shouldn't call this directly

typedef void (*likely_nullary_function)(likely_matrix *dst);
inline likely_nullary_function likely_make_nullary_function(const char *description)
{ return (likely_nullary_function)likely_make_function(description, 0); }

typedef void (*likely_unary_function)(const likely_matrix *src, likely_matrix *dst);
inline likely_unary_function likely_make_unary_function(const char *description)
{ return (likely_unary_function)likely_make_function(description, 1); }

typedef void (*likely_binary_function)(const likely_matrix *srcA, const likely_matrix *srcB, likely_matrix *dst);
inline likely_binary_function likely_make_binary_function(const char *description)
{ return (likely_binary_function)likely_make_function(description, 2); }

typedef void (*likely_ternary_function)(const likely_matrix *srcA, const likely_matrix *srcB, const likely_matrix *srcC, likely_matrix *dst);
inline likely_ternary_function likely_make_ternary_function(const char *description)
{ return (likely_ternary_function)likely_make_function(description, 3); }

#ifdef __cplusplus
}
#endif

// C++ wrapper starts here
#ifdef __cplusplus
#include <string>

namespace likely {

inline const char *indexHTML() { return likely_index_html(); }

struct Matrix : public likely_matrix
{
    Matrix() { likely_matrix_initialize_null(this); }
    Matrix(uint16_t hash) { likely_matrix_initialize(this, 0, 0, 0, 0, hash); }
    Matrix(uint32_t channels, uint32_t columns, uint32_t rows, uint32_t frames, uint16_t hash)
    { likely_matrix_initialize(this, channels, columns, rows, frames, hash); }

    inline int  depth() const { return likely_depth(this); }
    inline void setDepth(int bits) { likely_set_depth(this, bits); }
    inline bool isFloating() const { return likely_is_floating(this); }
    inline void setFloating(bool isFloating) { likely_set_floating(this, isFloating); }
    inline bool isSigned() const { return likely_is_signed(this); }
    inline void setSigned(bool isSigned) { likely_set_signed(this, isSigned); }
    inline int  type() const { return likely_type(this); }
    inline void setType(int type) { likely_set_type(this, type); }
    inline bool parallel() const { return likely_is_parallel(this); }
    inline void setParallel(bool parallel) { likely_set_parallel(this, parallel); }
    inline bool heterogeneous() const { return likely_is_heterogeneous(this); }
    inline void setHeterogeneous(bool heterogeneous) { likely_set_heterogeneous(this, heterogeneous); }
    inline bool isSingleChannel() const { return likely_is_single_channel(this); }
    inline void setSingleChannel(bool isSingleChannel) { likely_set_single_channel(this, isSingleChannel); }
    inline bool isSingleColumn() const { return likely_is_single_column(this); }
    inline void setSingleColumn(bool isSingleColumn) { likely_set_single_column(this, isSingleColumn); }
    inline bool isSingleRow() const { return likely_is_single_row(this); }
    inline void setSingleRow(bool isSingleRow) { likely_set_single_row(this, isSingleRow); }
    inline bool isSingleFrame() const { return likely_is_single_frame(this); }
    inline void setSingleFrame(bool isSingleFrame) { likely_set_single_frame(this, isSingleFrame); }
    inline uint32_t elements() const { return likely_elements(this); }
    inline uint32_t bytes() const { return likely_bytes(this); }
    inline double element(uint32_t c, uint32_t x, uint32_t y, uint32_t t) const { return likely_element(this, c, x, y, t); }
    inline void setElement(double value, uint32_t c, uint32_t x, uint32_t y, uint32_t t) { likely_set_element(this, value, c, x, y, t); }
};

typedef likely_nullary_function NullaryFunction;
inline NullaryFunction makeNullaryFunction(const char *description) { return likely_make_nullary_function(description); }

typedef likely_unary_function UnaryFunction;
inline UnaryFunction makeUnaryFunction(const char *description) { return likely_make_unary_function(description); }

typedef likely_binary_function BinaryFunction;
inline BinaryFunction makeBinaryFunction(const char *description) { return likely_make_binary_function(description); }

typedef likely_ternary_function TernaryFunction;
inline TernaryFunction makeTernaryFunction(const char *description) { return likely_make_ternary_function(description); }

} // namespace likely

#endif // __cplusplus

#endif // __LIKELY_H
