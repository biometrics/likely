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
#include <stdlib.h>
#include <string.h>

#include "likely/runtime.h"

//! [likely_bytes implementation.]
size_t likely_bytes(likely_const_mat mat)
{
    if (mat->type & likely_pointer)
        return 0;
    return ((mat->type & likely_depth) * (size_t) mat->channels * (size_t) mat->columns * (size_t) mat->rows * (size_t) mat->frames + 7) / 8;
}
//! [likely_bytes implementation.]

//! [likely_is_string implementation.]
bool likely_is_string(likely_const_mat m)
{
    return m
            && (m->type == (likely_i8 | likely_multi_channel))
            && (m->channels >= 1)
            && (m->columns  == 1)
            && (m->rows     == 1)
            && (m->frames   == 1)
            && !m->data[m->channels - 1] /* test for NULL-terminator */;
}
//! [likely_is_string implementation.]

//! [likely_new implementation.]
likely_mat likely_new(likely_type type, uint32_t channels, uint32_t columns, uint32_t rows, uint32_t frames, void const *data)
{
    // Note that we don't check the opposite case where an axis is one but the type suggests it is greater than one,
    // such a case is considered to be sub-optimal but correct.
    if ((channels > 1) && !(type & likely_multi_channel)) { assert(!"expected multi-channel type"); return NULL; }
    if ((columns  > 1) && !(type & likely_multi_column )) { assert(!"expected multi-column type" ); return NULL; }
    if ((rows     > 1) && !(type & likely_multi_row    )) { assert(!"expected multi-row type"    ); return NULL; }
    if ((frames   > 1) && !(type & likely_multi_frame  )) { assert(!"expected multi-frame type"  ); return NULL; }

    const size_t elements = (size_t)channels * (size_t)columns * (size_t)rows * (size_t)frames;
    if (elements == 0) { assert(!"expected non-zero elements"); return NULL; }

    const size_t bytes = ((type & likely_depth) * elements + 7) / 8;
    const unsigned char alignment = 16; // Likely guarantees that likely_matrix::data has 16-byte alignment
    likely_mat mat = (likely_mat) malloc(sizeof(struct likely_matrix) + bytes + alignment);
    if (!mat) { assert(!"malloc failure"); return NULL; }
    const char offset = alignment - ((uintptr_t)&mat->data % alignment);
    mat = (likely_mat) ((uintptr_t)mat + offset);
    ((unsigned char*)mat)[-1] = offset;
    assert((uintptr_t)&mat->data % alignment == 0);

    mat->ref_count = 1;
    mat->type = type;
    mat->channels = channels;
    mat->columns = columns;
    mat->rows = rows;
    mat->frames = frames;

    if (data)
        memcpy((void*)mat->data, data, bytes);

    return mat;
}
//! [likely_new implementation.]

//! [likely_scalar implementation.]
likely_mat likely_scalar(likely_type type, double *values, uint32_t n)
{
    likely_mat m = likely_new(type, n, 1, 1, 1, NULL);
    for (uint32_t i=0; i<n; i++)
        likely_set_element(m, values[i], i, 0, 0, 0);
    return m;
}
//! [likely_scalar implementation.]

//! [likely_string implementation.]
likely_mat likely_string(const char *str)
{
    return likely_new(likely_i8 | likely_multi_channel, (uint32_t)strlen(str) + 1 /* include the null-terminator */, 1, 1, 1, str);
}
//! [likely_string implementation.]

//! [likely_retain_mat implementation.]
likely_mat likely_retain_mat(likely_const_mat mat)
{
    if (!mat)
        return NULL;
    assert(mat->ref_count > 0);
    if (mat->ref_count != UINT32_MAX)
        ((likely_mat) mat)->ref_count++;
    return (likely_mat) mat;
}
//! [likely_retain_mat implementation.]

//! [likely_release_mat implementation.]
void likely_release_mat(likely_const_mat mat)
{
    if (!mat)
        return;
    assert(mat->ref_count > 0);
    if ((mat->ref_count == UINT32_MAX) || --((likely_mat) mat)->ref_count)
        return;
    free((void*) ((uintptr_t)mat - ((unsigned char*)mat)[-1]));
}
//! [likely_release_mat implementation.]

//! [likely_element implementation.]
double likely_get_element(likely_const_mat m, uint32_t c, uint32_t x, uint32_t y, uint32_t t)
{
    size_t columnStep, rowStep, frameStep, index;
    if (!m || (c >= m->channels) || (x >= m->columns) || (y >= m->rows) || (t >= m->frames))
        return NAN;

    columnStep = m->channels;
    rowStep = m->columns * columnStep;
    frameStep = m->rows * rowStep;
    index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (m->type & likely_c_type) {
      case likely_u8:  return (double) (( uint8_t const*) m->data)[index];
      case likely_u16: return (double) ((uint16_t const*) m->data)[index];
      case likely_u32: return (double) ((uint32_t const*) m->data)[index];
      case likely_u64: return (double) ((uint64_t const*) m->data)[index];
      case likely_i8:  return (double) ((  int8_t const*) m->data)[index];
      case likely_i16: return (double) (( int16_t const*) m->data)[index];
      case likely_i32: return (double) (( int32_t const*) m->data)[index];
      case likely_i64: return (double) (( int64_t const*) m->data)[index];
      case likely_f32: return (double) ((   float const*) m->data)[index];
      case likely_f64: return (double) ((  double const*) m->data)[index];
      case likely_u1:  return (double) ((((uint8_t const*)m->data)[index/8] & (1 << index%8)) != 0);
      default: assert(!"likely_element unsupported type");
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

    switch (m->type & likely_c_type) {
      case likely_u8:  (( uint8_t*)m->data)[index] = ( uint8_t)value; break;
      case likely_u16: ((uint16_t*)m->data)[index] = (uint16_t)value; break;
      case likely_u32: ((uint32_t*)m->data)[index] = (uint32_t)value; break;
      case likely_u64: ((uint64_t*)m->data)[index] = (uint64_t)value; break;
      case likely_i8:  ((  int8_t*)m->data)[index] = (  int8_t)value; break;
      case likely_i16: (( int16_t*)m->data)[index] = ( int16_t)value; break;
      case likely_i32: (( int32_t*)m->data)[index] = ( int32_t)value; break;
      case likely_i64: (( int64_t*)m->data)[index] = ( int64_t)value; break;
      case likely_f32: ((   float*)m->data)[index] = (   float)value; break;
      case likely_f64: ((  double*)m->data)[index] = (  double)value; break;
      case likely_u1:  if (value == 0) (((uint8_t*)m->data)[index/8] &= ~(1 << index%8));
                       else            (((uint8_t*)m->data)[index/8] |=  (1 << index%8)); break;
      default: assert(!"likely_set_element unsupported type");
    }
}
