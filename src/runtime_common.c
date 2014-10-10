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

void likely_assert(bool condition, const char *format, ...)
{
    va_list ap;
    if (condition)
        return;

    va_start(ap, format);
    fprintf(stderr, "Likely ");
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");

#ifdef _WIN32
    exit(EXIT_FAILURE); // We prefer not to trigger the Windows crash dialog box
#else // !_WIN32
    abort();
#endif // _WIN32
}

//! [likely_bytes implementation.]
size_t likely_bytes(likely_const_mat mat)
{
    if (mat->type & likely_matrix_array)
        return 0;
    return ((mat->type & likely_matrix_depth) * (size_t) mat->channels * (size_t) mat->columns * (size_t) mat->rows * (size_t) mat->frames + 7) / 8;
}
//! [likely_bytes implementation.]

//! [likely_new implementation.]
likely_mat likely_new(likely_matrix_type type, uint32_t channels, uint32_t columns, uint32_t rows, uint32_t frames, void const *data)
{
    likely_mat m;
    const size_t bytes = ((uint64_t)(type & likely_matrix_depth) * channels * columns * rows * frames + 7) / 8;
    m = (likely_mat) malloc(sizeof(struct likely_matrix) + bytes);
    m->ref_count = 1;
    m->type = type;
    m->channels = channels;
    m->columns = columns;
    m->rows = rows;
    m->frames = frames;

    if (channels > 1) m->type |= likely_matrix_multi_channel;
    if (columns  > 1) m->type |= likely_matrix_multi_column;
    if (rows     > 1) m->type |= likely_matrix_multi_row;
    if (frames   > 1) m->type |= likely_matrix_multi_frame;

    if (data)
        memcpy((void*)m->data, data, bytes);

    return m;
}
//! [likely_new implementation.]

likely_mat likely_scalar(likely_matrix_type type, double value)
{
    return likely_scalar_n(type, &value, 1);
}

//! [likely_scalar_n implementation.]
likely_mat likely_scalar_n(likely_matrix_type type, double *values, size_t n)
{
    likely_mat m = likely_new(type, n, 1, 1, 1, NULL);
    for (size_t i=0; i<n; i++)
        likely_set_element(m, values[i], i, 0, 0, 0);
    return m;
}
//! [likely_scalar_n implementation.]

likely_mat likely_scalar_va(likely_matrix_type type, double value, ...)
{
    double *values = NULL;
    int i = 0;

    va_list ap;
    va_start(ap, value);
    while (!isnan(value)) {
        if (i % 2 == 0)
            values = realloc(values, 2 * (i == 0 ? 1 : i) * sizeof(double));
        values[i++] = value;
        value = va_arg(ap, double);
    }
    va_end(ap);

    return likely_scalar_n(type, values, i);
}

//! [likely_string implementation.]
likely_mat likely_string(const char *str)
{
    return likely_new(likely_matrix_string, strlen(str) + 1 /* include the null-terminator */, 1, 1, 1, str);
}
//! [likely_string implementation.]

//! [likely_retain implementation.]
likely_mat likely_retain(likely_const_mat mat)
{
    if (!mat)
        return NULL;
    assert(mat->ref_count > 0);
    ((likely_mat) mat)->ref_count++;
    return (likely_mat) mat;
}
//! [likely_retain implementation.]

//! [likely_release implementation.]
void likely_release(likely_const_mat mat)
{
    if (!mat)
        return;
    assert(mat->ref_count > 0);
    if (--((likely_mat) mat)->ref_count)
        return;
    free((void*) mat);
}
//! [likely_release implementation.]

//! [likely_element implementation.]
double likely_element(likely_const_mat m, uint32_t c, uint32_t x, uint32_t y, uint32_t t)
{
    size_t columnStep, rowStep, frameStep, index;
    if (!m || (c >= m->channels) || (x >= m->columns) || (y >= m->rows) || (t >= m->frames))
        return NAN;

    columnStep = m->channels;
    rowStep = m->columns * columnStep;
    frameStep = m->rows * rowStep;
    index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (m->type & likely_matrix_c_type) {
      case likely_matrix_u8:  return (double) (( uint8_t const*) m->data)[index];
      case likely_matrix_u16: return (double) ((uint16_t const*) m->data)[index];
      case likely_matrix_u32: return (double) ((uint32_t const*) m->data)[index];
      case likely_matrix_u64: return (double) ((uint64_t const*) m->data)[index];
      case likely_matrix_i8:  return (double) ((  int8_t const*) m->data)[index];
      case likely_matrix_i16: return (double) (( int16_t const*) m->data)[index];
      case likely_matrix_i32: return (double) (( int32_t const*) m->data)[index];
      case likely_matrix_i64: return (double) (( int64_t const*) m->data)[index];
      case likely_matrix_f32: return (double) ((   float const*) m->data)[index];
      case likely_matrix_f64: return (double) ((  double const*) m->data)[index];
      case likely_matrix_u1:  return (double) ((((uint8_t const*)m->data)[index/8] & (1 << index%8)) != 0);
      default: likely_assert(false, "likely_element unsupported type");
    }
    return NAN;
}
//! [likely_element implementation.]

void likely_set_element(likely_mat m, double value, uint32_t c, uint32_t x, uint32_t y, uint32_t t)
{
    size_t columnStep, rowStep, frameStep, index;
    if (!m || (c >= m->channels) || (x >= m->columns) || (y >= m->rows) || (t >= m->frames))
        return;

    columnStep = m->channels;
    rowStep = m->columns * columnStep;
    frameStep = m->rows * rowStep;
    index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (m->type & likely_matrix_c_type) {
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

bool likely_is_string(likely_const_mat m)
{
    return m && (m->type == likely_matrix_string) && !m->data[m->channels * m->columns * m->rows * m->frames - 1];
}
