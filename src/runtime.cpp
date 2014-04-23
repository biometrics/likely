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

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <mutex>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

#include "likely/runtime.h"

#define LIKELY_NUM_ARITIES 4

using namespace std;

void likely_assert(bool condition, const char *format, ...)
{
    if (condition) return;
    va_list ap;
    va_start(ap, format);
    fprintf(stderr, "Likely ");
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    abort();
}

size_t likely_get(size_t type, size_t mask) { return type & mask; }
void likely_set(size_t *type, size_t value, size_t mask) { *type &= ~mask; *type |= value & mask; }
bool likely_get_bool(size_t type, size_t mask) { return (type & mask) != 0; }
void likely_set_bool(size_t *type, bool value, size_t mask) { value ? *type |= mask : *type &= ~mask; }

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
static likely_mat recycled = NULL;

likely_mat likely_new(likely_type type, likely_size channels, likely_size columns, likely_size rows, likely_size frames, void const *data)
{
    likely_mat m;
    const size_t dataBytes = (uint64_t(likely_depth(type)) * channels * columns * rows * frames + 7) / 8;
    const size_t bytes = sizeof(likely_matrix) + dataBytes;
    if (recycled) {
        if (recycled->bytes >= bytes) {
            m = recycled;
        } else {
            m = (likely_mat) realloc((void*) recycled, bytes);
            m->bytes = bytes;
        }
        recycled = NULL;
    } else {
        m = (likely_mat) malloc(bytes);
        m->bytes = bytes;
    }

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
        memcpy((void*) &m->data, data, dataBytes);

    return m;
}

likely_mat likely_scalar(likely_type type, double value)
{
    return likely_scalars(type, value, numeric_limits<double>::quiet_NaN());
}

likely_mat likely_scalars(likely_type type, double value, ...)
{
    vector<double> values;
    va_list ap;
    va_start(ap, value);
    while (!isnan(value)) {
        values.push_back(value);
        value = va_arg(ap, double);
    }
    va_end(ap);

    if (type == likely_matrix_void)
        for (size_t i=0; i<values.size(); i++)
            type = likely_type_from_types(type, likely_type_from_value(values[i]));

    likely_mat m = likely_new(type, values.size(), 1, 1, 1, NULL);
    for (size_t i=0; i<values.size(); i++)
        likely_set_element(m, values[i], i, 0, 0, 0);
    return m;
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
    if (m) ++const_cast<likely_mat>(m)->ref_count;
    return (likely_mat) m;
}

void likely_release(likely_const_mat m)
{
    if (!m || --const_cast<likely_mat>(m)->ref_count) return;
    if (recycled) {
        if (m->bytes > recycled->bytes) {
            likely_mat tmp = recycled;
            recycled = const_cast<likely_mat>(m);
            free(tmp);
        } else {
            free((void*) m);
        }
    } else {
        recycled = const_cast<likely_mat>(m);
    }
}

double likely_element(likely_const_mat m, likely_size c, likely_size x, likely_size y, likely_size t)
{
    if (!m) return numeric_limits<double>::quiet_NaN();
    const likely_size columnStep = m->channels;
    const likely_size rowStep = m->columns * columnStep;
    const likely_size frameStep = m->rows * rowStep;
    const likely_size index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (likely_data(m->type)) {
      case likely_matrix_u8:  return double((( uint8_t const*)m->data)[index]);
      case likely_matrix_u16: return double(((uint16_t const*)m->data)[index]);
      case likely_matrix_u32: return double(((uint32_t const*)m->data)[index]);
      case likely_matrix_u64: return double(((uint64_t const*)m->data)[index]);
      case likely_matrix_i8:  return double(((  int8_t const*)m->data)[index]);
      case likely_matrix_i16: return double((( int16_t const*)m->data)[index]);
      case likely_matrix_i32: return double((( int32_t const*)m->data)[index]);
      case likely_matrix_i64: return double((( int64_t const*)m->data)[index]);
      case likely_matrix_f32: return double(((   float const*)m->data)[index]);
      case likely_matrix_f64: return double(((  double const*)m->data)[index]);
      case likely_matrix_u1:  return double((((uint8_t const*)m->data)[index/8] & (1 << index%8)) != 0);
      default: assert(!"likely_element unsupported type");
    }
    return numeric_limits<double>::quiet_NaN();
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
                            else              (((uint8_t*)m->data)[index/8] |=  (1 << index%8)); break;
      default: assert(!"likely_set_element unsupported type");
    }
}

likely_mat likely_type_to_string(likely_type type)
{
    stringstream typeStream;
    typeStream << (likely_floating(type) ? "f" : (likely_signed(type) ? "i" : "u"));
    typeStream << likely_depth(type);
    if (likely_parallel(type))       typeStream << "P";
    if (likely_heterogeneous(type))  typeStream << "H";
    if (likely_multi_channel(type))  typeStream << "C";
    if (likely_multi_column(type))   typeStream << "X";
    if (likely_multi_row(type))      typeStream << "Y";
    if (likely_multi_frame(type))    typeStream << "T";
    if (likely_saturation(type))     typeStream << "S";
    return likely_string(typeStream.str().c_str());
}

likely_mat likely_type_field_to_string(likely_type type)
{
    if (type == likely_matrix_void           ) return likely_string("void");
    if (type == likely_matrix_depth          ) return likely_string("depth");
    if (type == likely_matrix_signed         ) return likely_string("signed");
    if (type == likely_matrix_floating       ) return likely_string("floating");
    if (type == likely_matrix_data           ) return likely_string("data");
    if (type == likely_matrix_parallel       ) return likely_string("parallel");
    if (type == likely_matrix_heterogeneous  ) return likely_string("heterogeneous");
    if (type == likely_matrix_execution      ) return likely_string("exeuction");
    if (type == likely_matrix_multi_channel  ) return likely_string("multi_channel");
    if (type == likely_matrix_multi_column   ) return likely_string("multi_column");
    if (type == likely_matrix_multi_row      ) return likely_string("multi_row");
    if (type == likely_matrix_multi_frame    ) return likely_string("multi_frame");
    if (type == likely_matrix_multi_dimension) return likely_string("multi_dimension");
    if (type == likely_matrix_saturation     ) return likely_string("saturation");
    return likely_type_to_string(type);
}

likely_type likely_type_from_string(const char *str)
{
    const size_t len = strlen(str);
    if (len == 0) return likely_matrix_void;

    likely_type t;
    if      (str[0] == 'f') t = likely_matrix_signed | likely_matrix_floating;
    else if (str[0] == 'i') t = likely_matrix_signed;
    else if (str[0] == 'u') t = likely_matrix_void;
    else                    return likely_matrix_void;

    char *rem;
    t += (int)strtol(str+1, &rem, 10);

    while (*rem) {
        if      (*rem == 'P') t |= likely_matrix_parallel;
        else if (*rem == 'H') t |= likely_matrix_heterogeneous;
        else if (*rem == 'C') t |= likely_matrix_multi_channel;
        else if (*rem == 'X') t |= likely_matrix_multi_column;
        else if (*rem == 'Y') t |= likely_matrix_multi_row;
        else if (*rem == 'T') t |= likely_matrix_multi_frame;
        else if (*rem == 'S') t |= likely_matrix_saturation;
        else                  return likely_matrix_void;
        rem++;
    }

    return t;
}

likely_type likely_type_field_from_string(const char *str, bool *ok)
{
    if (ok) *ok = true;
    if (!strcmp(str, "void"           )) return likely_matrix_void;
    if (!strcmp(str, "depth"          )) return likely_matrix_depth;
    if (!strcmp(str, "signed"         )) return likely_matrix_signed;
    if (!strcmp(str, "floating"       )) return likely_matrix_floating;
    if (!strcmp(str, "data"           )) return likely_matrix_data;
    if (!strcmp(str, "parallel"       )) return likely_matrix_parallel;
    if (!strcmp(str, "heterogeneous"  )) return likely_matrix_heterogeneous;
    if (!strcmp(str, "execution"      )) return likely_matrix_execution;
    if (!strcmp(str, "multi_channel"  )) return likely_matrix_multi_channel;
    if (!strcmp(str, "multi_column"   )) return likely_matrix_multi_column;
    if (!strcmp(str, "multi_row"      )) return likely_matrix_multi_row;
    if (!strcmp(str, "multi_frame"    )) return likely_matrix_multi_frame;
    if (!strcmp(str, "multi_dimension")) return likely_matrix_multi_dimension;
    if (!strcmp(str, "saturation"     )) return likely_matrix_saturation;
    likely_type type = likely_type_from_string(str);
    if (ok) *ok = (type != likely_matrix_void);
    return type;
}

likely_type likely_type_from_value(double value)
{
    if (value >= 0) {
        if (uint32_t(value) == value) return likely_matrix_u32;
        if (uint64_t(value) == value) return likely_matrix_u64;
    } else {
        if (int32_t(value) == value) return likely_matrix_i32;
        if (int64_t(value) == value) return likely_matrix_i64;
    }
    if (float(value) == value) return likely_matrix_f32;
    else                       return likely_matrix_f64;
}

likely_type likely_type_from_types(likely_type lhs, likely_type rhs)
{
    likely_type result = lhs | rhs;
    likely_set_depth(&result, max(likely_depth(lhs), likely_depth(rhs)));
    return result;
}

// Parallel synchronization
static condition_variable worker;
static mutex work;
static atomic<bool> *workers = NULL;
static size_t numWorkers = 0;

// Parallel data
static void *currentThunk = NULL;
static likely_arity thunkArity = 0;
static likely_size thunkSize = 0;
static likely_const_mat thunkMatricies[LIKELY_NUM_ARITIES+1];

static void executeWorker(size_t id)
{
    // There are hardware_concurrency-1 helper threads and the main thread with id = 0
    const likely_size step = (thunkSize+numWorkers-1)/numWorkers;
    const likely_size start = id * step;
    const likely_size stop = std::min((id+1)*step, thunkSize);
    if (start >= stop) return;

    // Final three parameters are: dst, start, stop
    typedef void (*likely_kernel_0)(likely_const_mat, likely_size, likely_size);
    typedef void (*likely_kernel_1)(const likely_const_mat, likely_const_mat, likely_size, likely_size);
    typedef void (*likely_kernel_2)(const likely_const_mat, const likely_const_mat, likely_const_mat, likely_size, likely_size);
    typedef void (*likely_kernel_3)(const likely_const_mat, const likely_const_mat, const likely_const_mat, likely_const_mat, likely_size, likely_size);

    switch (thunkArity) {
      case 0: reinterpret_cast<likely_kernel_0>(currentThunk)((likely_const_mat)thunkMatricies[0], start, stop); break;
      case 1: reinterpret_cast<likely_kernel_1>(currentThunk)(thunkMatricies[0], (likely_const_mat)thunkMatricies[1], start, stop); break;
      case 2: reinterpret_cast<likely_kernel_2>(currentThunk)(thunkMatricies[0], thunkMatricies[1], (likely_const_mat)thunkMatricies[2], start, stop); break;
      case 3: reinterpret_cast<likely_kernel_3>(currentThunk)(thunkMatricies[0], thunkMatricies[1], thunkMatricies[2], (likely_const_mat)thunkMatricies[3], start, stop); break;
      default: likely_assert(false, "executeWorker invalid arity: %d", thunkArity);
    }
}

static void workerThread(size_t id)
{
    while (true) {
        {
            unique_lock<mutex> lock(work);
            workers[id] = false;
            while (!workers[id])
                worker.wait(lock);
        }

        executeWorker(id);
    }
}

extern "C" LIKELY_EXPORT void likely_fork(void *thunk, likely_arity arity, likely_size size, likely_const_mat src, ...)
{
    static mutex forkLock;
    lock_guard<mutex> lockFork(forkLock);

    // Spin up the worker threads
    if (workers == NULL) {
        numWorkers = std::max((int)thread::hardware_concurrency(), 1);
        workers = new atomic<bool>[numWorkers];
        for (size_t i = 1; i < numWorkers; i++) {
            workers[i] = true;
            thread(workerThread, i).detach();
            while (workers[i]) {} // Wait for the worker to initialize
        }

        // An impossible case used to ensure that `likely_fork` isn't stripped when optimizing executable size
        if (workers == NULL)
            likely_fork(NULL, 0, 0, NULL);
    }

    currentThunk = thunk;
    thunkArity = arity;
    thunkSize = size;

    va_list ap;
    va_start(ap, src);
    for (int i=0; i<arity+1; i++) {
        thunkMatricies[i] = src;
        src = va_arg(ap, likely_const_mat);
    }
    va_end(ap);

    {
        unique_lock<mutex> lock(work);
        for (size_t i = 1; i < numWorkers; i++) {
            assert(!workers[i]);
            workers[i] = true;
        }
    }

    worker.notify_all();
    executeWorker(0);

    for (size_t i = 1; i < numWorkers; i++)
        while (workers[i]) {} // Wait for the worker to finish
}
