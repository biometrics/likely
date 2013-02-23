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

/* C API Starts Here */
#ifdef __cplusplus
extern "C" {
#endif

LIKELY_EXPORT const char *likely_index_html();

struct likely_matrix
{
    uint8_t *data;
    uint32_t channels, columns, rows, frames;
    uint16_t hash; /* Bits : 8
                      Floating : 1
                      Signed : 1
                      (Reserved) : 2
                      Single-channel : 1
                      Single-column : 1
                      Single-row : 1
                      Single-frame : 1 */

    enum Hash { Bits = 0x00FF,
                Floating = 0x0100,
                Signed = 0x0200,
                SingleChannel = 0x1000,
                SingleColumn = 0x2000,
                SingleRow = 0x4000,
                SingleFrame = 0x8000,
                u1  = 1,
                u8  = 8,
                u16 = 16,
                u32 = 32,
                u64 = 64,
                s8  = 8  + Signed,
                s16 = 16 + Signed,
                s32 = 32 + Signed,
                s64 = 64 + Signed,
                f16 = 16 + Floating + Signed,
                f32 = 32 + Floating + Signed,
                f64 = 64 + Floating + Signed };
};

LIKELY_EXPORT int  likely_bits(const likely_matrix *m);
LIKELY_EXPORT void likely_set_bits(likely_matrix *m, int bits);
LIKELY_EXPORT bool likely_is_floating(const likely_matrix *m);
LIKELY_EXPORT void likely_set_floating(likely_matrix *m, bool is_floating);
LIKELY_EXPORT bool likely_is_signed(const likely_matrix *m);
LIKELY_EXPORT void likely_set_signed(likely_matrix *m, bool is_signed);
LIKELY_EXPORT int  likely_type(const likely_matrix *m);
LIKELY_EXPORT void likely_set_type(likely_matrix *m, int type);
LIKELY_EXPORT bool likely_is_single_channel(const likely_matrix *m);
LIKELY_EXPORT void likely_set_single_channel(likely_matrix *m, bool is_single_channel);
LIKELY_EXPORT bool likely_is_single_column(const likely_matrix *m);
LIKELY_EXPORT void likely_set_single_column(likely_matrix *m, bool is_single_column);
LIKELY_EXPORT bool likely_is_single_row(const likely_matrix *m);
LIKELY_EXPORT void likely_set_single_row(likely_matrix *m, bool is_single_row);
LIKELY_EXPORT bool likely_is_single_frame(const likely_matrix *m);
LIKELY_EXPORT void likely_set_single_frame(likely_matrix *m, bool is_single_frame);
LIKELY_EXPORT uint32_t likely_elements(const likely_matrix *m);
LIKELY_EXPORT uint32_t likely_bytes(const likely_matrix *m);

typedef void (*likely_unary_function)(const likely_matrix *src, likely_matrix *dst);
LIKELY_EXPORT likely_unary_function likely_make_unary_function(const char *description);

typedef void (*likely_binary_function)(const likely_matrix *srcA, const likely_matrix *srcB, likely_matrix *dst);
LIKELY_EXPORT likely_binary_function likely_make_binary_function(const char *description);

#ifdef __cplusplus
}
#endif

/* C++ Wrapper Starts Here */
#ifdef __cplusplus
#include <string>

namespace likely {

std::string indexHtml() { return likely_index_html(); }

struct Matrix : public likely_matrix
{
    Matrix()
    {
        data = NULL;
        channels = columns = rows = frames = hash = 0;
    }

    Matrix(uint32_t _channels, uint32_t _columns, uint32_t _rows, uint32_t _frames, uint16_t _hash)
    {
        data = NULL;
        channels = _channels;
        columns = _columns;
        rows = _rows;
        frames = _frames;
        hash = _hash;
        setSingleChannel(channels == 1);
        setSingleColumn(columns == 1);
        setSingleRow(rows == 1);
        setSingleFrame(frames == 1);
    }

    inline int  bits() const { return likely_bits(this); }
    inline void setBits(int bits) { likely_set_bits(this, bits); }
    inline bool isFloating() const { return likely_is_floating(this); }
    inline void setFloating(bool isFloating) { likely_set_floating(this, isFloating); }
    inline bool isSigned() const { return likely_is_signed(this); }
    inline void setSigned(bool isSigned) { likely_set_signed(this, isSigned); }
    inline int  type() const { return likely_type(this); }
    inline void setType(int type) { likely_set_type(this, type); }
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
};

typedef likely_unary_function UnaryFunction;
typedef likely_binary_function BinaryFunction;
inline UnaryFunction makeUnaryFunction(const char *description) { return likely_make_unary_function(description); }
inline BinaryFunction makeBinaryFunction(const char *description) { return likely_make_binary_function(description); }

} // namespace likely

#endif // __cplusplus

#endif // __LIKELY_H
