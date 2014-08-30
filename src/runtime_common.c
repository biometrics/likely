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

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "likely/runtime.h"

// Until Microsoft implements isnan
#if _MSC_VER
#define isnan _isnan
#endif

void likely_assert(bool condition, const char *format, ...)
{
    if (condition)
        return;

    va_list ap;
    va_start(ap, format);
    fprintf(stderr, "Likely ");
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

size_t likely_get(size_t type, size_t mask) { return type & mask; }
void likely_set(size_t *type, size_t value, size_t mask) { *type &= ~mask; *type |= value & mask; }
bool likely_get_bool(size_t type, size_t mask) { return (type & mask) != 0; }
void likely_set_bool(size_t *type, bool value, size_t mask) { if (value) *type |= mask; else *type &= ~mask; }

size_t likely_depth(likely_type type) { return likely_get(type, likely_matrix_depth); }
void likely_set_depth(likely_type *type, size_t depth) { likely_set(type, depth, likely_matrix_depth); }
bool likely_signed(likely_type type) { return likely_get_bool(type, likely_matrix_signed); }
void likely_set_signed(likely_type *type, bool signed_) { likely_set_bool(type, signed_, likely_matrix_signed); }
bool likely_floating(likely_type type) { return likely_get_bool(type, likely_matrix_floating); }
void likely_set_floating(likely_type *type, bool floating) { likely_set_bool(type, floating, likely_matrix_floating); }
likely_type likely_data(likely_type type) { return likely_get(type, likely_matrix_data); }
void likely_set_data(likely_type *type, likely_type data) { likely_set(type, data, likely_matrix_data); }
bool likely_parallel(likely_type type) { return likely_get_bool(type, likely_matrix_parallel); }
void likely_set_parallel(likely_type *type, bool parallel) { likely_set_bool(type, parallel, likely_matrix_parallel); }
bool likely_heterogeneous(likely_type type) { return likely_get_bool(type, likely_matrix_heterogeneous); }
void likely_set_heterogeneous(likely_type *type, bool heterogeneous) { likely_set_bool(type, heterogeneous, likely_matrix_heterogeneous); }
likely_type likely_execution(likely_type type) { return likely_get(type, likely_matrix_execution); }
void likely_set_execution(likely_type *type, likely_type execution) { likely_set(type, execution, likely_matrix_execution); }
bool likely_multi_channel(likely_type type) { return likely_get_bool(type, likely_matrix_multi_channel); }
void likely_set_multi_channel(likely_type *type, bool multi_channel) { likely_set_bool(type, multi_channel, likely_matrix_multi_channel); }
bool likely_multi_column(likely_type type) { return likely_get_bool(type, likely_matrix_multi_column); }
void likely_set_multi_column(likely_type *type, bool multi_column) { likely_set_bool(type, multi_column, likely_matrix_multi_column); }
bool likely_multi_row(likely_type type) { return likely_get_bool(type, likely_matrix_multi_row); }
void likely_set_multi_row(likely_type *type, bool multi_row) { likely_set_bool(type, multi_row, likely_matrix_multi_row); }
bool likely_multi_frame(likely_type type) { return likely_get_bool(type, likely_matrix_multi_frame); }
void likely_set_multi_frame(likely_type *type, bool multi_frame) { likely_set_bool(type, multi_frame, likely_matrix_multi_frame); }
likely_type likely_multi_dimension(likely_type type) { return likely_get(type, likely_matrix_multi_dimension); }
void likely_set_multi_dimension(likely_type *type, likely_type multi_dimension) { likely_set(type, multi_dimension, likely_matrix_multi_dimension); }
bool likely_saturation(likely_type type) { return likely_get_bool(type, likely_matrix_saturation); }
void likely_set_saturation(likely_type *type, bool saturation) { likely_set_bool(type, saturation, likely_matrix_saturation); }
size_t likely_magic(likely_type type) { return likely_get(type, likely_matrix_magic); }
void likely_set_magic(likely_type *type, size_t magic) { likely_set(type, magic, likely_matrix_magic); }

likely_size likely_elements(likely_const_mat m)
{
    return m->channels * m->columns * m->rows * m->frames;
}

likely_size likely_bytes(likely_const_mat m)
{
    return (likely_depth(m->type) * likely_elements(m) + 7) / 8;
}

// TODO: make this thread_local when compiler support improves
likely_mat likely_new(likely_type type, likely_size channels, likely_size columns, likely_size rows, likely_size frames, void const *data)
{
    likely_mat m;
    const size_t dataBytes = (((uint64_t)likely_depth(type)) * channels * columns * rows * frames + 7) / 8;
    const size_t bytes = sizeof(struct likely_matrix) + dataBytes;
    m = (likely_mat) malloc(bytes);
    m->bytes = bytes;
    m->ref_count = 1;
    m->type = type | likely_matrix_matrix;
    m->channels = channels;
    m->columns = columns;
    m->rows = rows;
    m->frames = frames;

    likely_set_multi_channel(&m->type, channels > 1);
    likely_set_multi_column(&m->type, columns > 1);
    likely_set_multi_row(&m->type, rows > 1);
    likely_set_multi_frame(&m->type, frames > 1);

    if (data)
        memcpy((void*)m->data, data, dataBytes);

    return m;
}

likely_mat likely_scalar(likely_type type, double value)
{
    return likely_scalar_n(type, &value, 1);
}

likely_mat likely_scalar_n(likely_type type, double *values, size_t n)
{
    likely_mat m = likely_new(type, n, 1, 1, 1, NULL);
    for (size_t i=0; i<n; i++)
        likely_set_element(m, values[i], i, 0, 0, 0);
    return m;
}

likely_mat likely_scalar_va(likely_type type, double value, ...)
{
    int count = 0;
    double *values;
    {
        va_list ap;
        va_start(ap, value);
        while (!isnan(value))
            count++;
        va_end(ap);
        values = (double*) alloca(count * sizeof(double));
    }

    int i = 0;
    va_list ap;
    va_start(ap, value);
    while (!isnan(value)) {
        values[i++] = value;
        value = va_arg(ap, double);
    }
    va_end(ap);

    return likely_scalar_n(type, values, count);
}

likely_mat likely_string(const char *str)
{
    return likely_new(likely_matrix_i8, strlen(str)+1, 1, 1, 1, str);
}

likely_mat likely_void()
{
    return likely_new(likely_matrix_void, 0, 0, 0, 0, NULL);
}

likely_mat likely_copy(likely_const_mat m)
{
    return likely_new(m->type, m->channels, m->columns, m->rows, m->frames, m->data);
}

likely_mat likely_retain(likely_const_mat m)
{
    if (m) ++((likely_mat) m)->ref_count;
    return (likely_mat) m;
}

void likely_release(likely_const_mat m)
{
    if (!m || --((likely_mat) m)->ref_count) return;
    free((void*) m);
}

double likely_element(likely_const_mat m, likely_size c, likely_size x, likely_size y, likely_size t)
{
    if (!m) return NAN;
    const likely_size columnStep = m->channels;
    const likely_size rowStep = m->columns * columnStep;
    const likely_size frameStep = m->rows * rowStep;
    const likely_size index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (likely_data(m->type)) {
      case likely_matrix_u8:  return (double) (( uint8_t const*) m->data)[index];
      case likely_matrix_u16: return (double) ((uint16_t const*)m->data)[index];
      case likely_matrix_u32: return (double) ((uint32_t const*)m->data)[index];
      case likely_matrix_u64: return (double) ((uint64_t const*)m->data)[index];
      case likely_matrix_i8:  return (double) ((  int8_t const*)m->data)[index];
      case likely_matrix_i16: return (double) (( int16_t const*)m->data)[index];
      case likely_matrix_i32: return (double) (( int32_t const*)m->data)[index];
      case likely_matrix_i64: return (double) (( int64_t const*)m->data)[index];
      case likely_matrix_f32: return (double) ((   float const*)m->data)[index];
      case likely_matrix_f64: return (double) ((  double const*)m->data)[index];
      case likely_matrix_u1:  return (double) ((((uint8_t const*)m->data)[index/8] & (1 << index%8)) != 0);
      default: assert(!"likely_element unsupported type");
    }
    return NAN;
}

void likely_set_element(likely_mat m, double value, likely_size c, likely_size x, likely_size y, likely_size t)
{
    if (!m) return;
    const likely_size columnStep = m->channels;
    const likely_size rowStep = m->columns * columnStep;
    const likely_size frameStep = m->rows * rowStep;
    const likely_size index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (likely_data(m->type)) {
      case likely_matrix_u8:  (( uint8_t*)m->data)[index] = ( uint8_t)value; break;
      case likely_matrix_u16: ((uint16_t*)m->data)[index] = (uint16_t)value; break;
      case likely_matrix_u32: ((uint32_t*)m->data)[index] = (uint32_t)value; break;
      case likely_matrix_u64: ((uint64_t*)m->data)[index] = (uint64_t)value; break;
      case likely_matrix_i8:  ((  int8_t*)m->data)[index] = (  int8_t)value; break;
      case likely_matrix_i16: (( int16_t*)m->data)[index] = ( int16_t)value; break;
      case likely_matrix_i32: (( int32_t*)m->data)[index] = ( int32_t)value; break;
      case likely_matrix_i64: (( int64_t*)m->data)[index] = ( int64_t)value; break;
      case likely_matrix_f32: ((   float*)m->data)[index] = (   float)value; break;
      case likely_matrix_f64: ((  double*)m->data)[index] = (  double)value; break;
      case likely_matrix_u1:  if (value == 0) (((uint8_t*)m->data)[index/8] &= ~(1 << index%8));
                              else            (((uint8_t*)m->data)[index/8] |=  (1 << index%8)); break;
      default: assert(!"likely_set_element unsupported type");
    }
}
