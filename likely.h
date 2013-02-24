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

struct likely_matrix
{
    uint8_t *data;
    uint32_t channels, columns, rows, frames;
    uint16_t hash; /* Depth : 8
                      Floating : 1
                      Signed : 1
                      OpenMP : 1
                      OpenCL : 1
                      Single-channel : 1
                      Single-column : 1
                      Single-row : 1
                      Single-frame : 1 */

    enum Hash { Depth = 0x00FF,
                Floating = 0x0100,
                Signed = 0x0200,
                Type = Depth | Floating | Signed,
                OpenMP = 0x0400,
                OpenCL = 0x0800,
                SingleChannel = 0x1000,
                SingleColumn = 0x2000,
                SingleRow = 0x4000,
                SingleFrame = 0x8000,
                u1  = 1,
                u8  = 8,
                u16 = 16,
                u32 = 32,
                u64 = 64,
                s8  = 8  | Signed,
                s16 = 16 | Signed,
                s32 = 32 | Signed,
                s64 = 64 | Signed,
                f16 = 16 | Floating | Signed,
                f32 = 32 | Floating | Signed,
                f64 = 64 | Floating | Signed };
};

// Convenience functions for default initializing a matrix
LIKELY_EXPORT void likely_matrix_initialize_null(likely_matrix *m);
LIKELY_EXPORT void likely_matrix_initialize(likely_matrix *m, uint32_t channels, uint32_t columns, uint32_t rows, uint32_t frames, uint16_t hash);

// Convenience functions for querying and editing the hash
LIKELY_EXPORT int  likely_depth(const likely_matrix *m);
LIKELY_EXPORT void likely_set_depth(likely_matrix *m, int bits);
LIKELY_EXPORT bool likely_is_floating(const likely_matrix *m);
LIKELY_EXPORT void likely_set_floating(likely_matrix *m, bool is_floating);
LIKELY_EXPORT bool likely_is_signed(const likely_matrix *m);
LIKELY_EXPORT void likely_set_signed(likely_matrix *m, bool is_signed);
LIKELY_EXPORT int  likely_type(const likely_matrix *m);
LIKELY_EXPORT void likely_set_type(likely_matrix *m, int type);
LIKELY_EXPORT bool likely_openmp(const likely_matrix *m);
LIKELY_EXPORT void likely_set_openmp(likely_matrix *m, bool openmp);
LIKELY_EXPORT bool likely_opencl(const likely_matrix *m);
LIKELY_EXPORT void likely_use_opencl(likely_matrix *m, bool opencl);
LIKELY_EXPORT bool likely_is_single_channel(const likely_matrix *m);
LIKELY_EXPORT void likely_set_single_channel(likely_matrix *m, bool is_single_channel);
LIKELY_EXPORT bool likely_is_single_column(const likely_matrix *m);
LIKELY_EXPORT void likely_set_single_column(likely_matrix *m, bool is_single_column);
LIKELY_EXPORT bool likely_is_single_row(const likely_matrix *m);
LIKELY_EXPORT void likely_set_single_row(likely_matrix *m, bool is_single_row);
LIKELY_EXPORT bool likely_is_single_frame(const likely_matrix *m);
LIKELY_EXPORT void likely_set_single_frame(likely_matrix *m, bool is_single_frame);

// Convenience functions for determining matrix size
LIKELY_EXPORT uint32_t likely_elements(const likely_matrix *m);
LIKELY_EXPORT uint32_t likely_bytes(const likely_matrix *m);

// Convenience functions for element access
// By convention c = channel, x = column, y = row, t = frame
LIKELY_EXPORT double   likely_element(const likely_matrix *m, uint32_t c, uint32_t x, uint32_t y, uint32_t t);
LIKELY_EXPORT void likely_set_element(likely_matrix *m, uint32_t c, uint32_t x, uint32_t y, uint32_t t, double value);

// Core functions for requesting kernels
typedef void (*likely_unary_function)(const likely_matrix *src, likely_matrix *dst);
LIKELY_EXPORT likely_unary_function likely_make_unary_function(const char *description);

typedef void (*likely_binary_function)(const likely_matrix *srcA, const likely_matrix *srcB, likely_matrix *dst);
LIKELY_EXPORT likely_binary_function likely_make_binary_function(const char *description);

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
    inline bool openMP() const { return likely_openmp(this); }
    inline void setOpenMP(bool OpenMP) { likely_set_openmp(this, OpenMP); }
    inline bool openCL() const { return likely_opencl(this); }
    inline void setOpenCL(bool OpenCL) { likely_use_opencl(this, OpenCL); }
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
    inline void setElement(uint32_t c, uint32_t x, uint32_t y, uint32_t t, double value) { likely_set_element(this, c, x, y, t, value); }
};

typedef likely_unary_function UnaryFunction;
typedef likely_binary_function BinaryFunction;
inline UnaryFunction makeUnaryFunction(const char *description) { return likely_make_unary_function(description); }
inline BinaryFunction makeBinaryFunction(const char *description) { return likely_make_binary_function(description); }

} // namespace likely

#endif // __cplusplus

#endif // __LIKELY_H
