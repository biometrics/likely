#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <sstream>
#include <thread>

#include "likely/likely_runtime.h"

#define LIKELY_NUM_ARITIES 4

using namespace std;

typedef struct likely_matrix_private
{
    size_t data_bytes;
    int ref_count;
} likely_matrix_private;

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

static int likely_get(likely_type type, likely_type_field mask) { return type & mask; }
static void likely_set(likely_type *type, int i, likely_type_field mask) { *type &= ~mask; *type |= i & mask; }
static bool likely_get_bool(likely_type type, likely_type_field mask) { return (type & mask) != 0; }
static void likely_set_bool(likely_type *type, bool b, likely_type_field mask) { b ? *type |= mask : *type &= ~mask; }

int  likely_depth(likely_type type) { return likely_get(type, likely_type_depth); }
void likely_set_depth(likely_type *type, int depth) { likely_set(type, depth, likely_type_depth); }
bool likely_signed(likely_type type) { return likely_get_bool(type, likely_type_signed); }
void likely_set_signed(likely_type *type, bool signed_) { likely_set_bool(type, signed_, likely_type_signed); }
bool likely_floating(likely_type type) { return likely_get_bool(type, likely_type_floating); }
void likely_set_floating(likely_type *type, bool floating) { likely_set_bool(type, floating, likely_type_floating); }
bool likely_parallel(likely_type type) { return likely_get_bool(type, likely_type_parallel); }
void likely_set_parallel(likely_type *type, bool parallel) { likely_set_bool(type, parallel, likely_type_parallel); }
bool likely_heterogeneous(likely_type type) { return likely_get_bool(type, likely_type_heterogeneous); }
void likely_set_heterogeneous(likely_type *type, bool heterogeneous) { likely_set_bool(type, heterogeneous, likely_type_heterogeneous); }
bool likely_multi_channel(likely_type type) { return likely_get_bool(type, likely_type_multi_channel); }
void likely_set_multi_channel(likely_type *type, bool multi_channel) { likely_set_bool(type, multi_channel, likely_type_multi_channel); }
bool likely_multi_column(likely_type type) { return likely_get_bool(type, likely_type_multi_column); }
void likely_set_multi_column(likely_type *type, bool multi_column) { likely_set_bool(type, multi_column, likely_type_multi_column); }
bool likely_multi_row(likely_type type) { return likely_get_bool(type, likely_type_multi_row); }
void likely_set_multi_row(likely_type *type, bool multi_row) { likely_set_bool(type, multi_row, likely_type_multi_row); }
bool likely_multi_frame(likely_type type) { return likely_get_bool(type, likely_type_multi_frame); }
void likely_set_multi_frame(likely_type *type, bool multi_frame) { likely_set_bool(type, multi_frame, likely_type_multi_frame); }
bool likely_saturation(likely_type type) { return likely_get_bool(type, likely_type_saturation); }
void likely_set_saturation(likely_type *type, bool saturation) { likely_set_bool(type, saturation, likely_type_saturation); }
int  likely_reserved(likely_type type) { return likely_get(type, likely_type_reserved); }
void likely_set_reserved(likely_type *type, int reserved) { likely_set(type, reserved, likely_type_reserved); }

likely_size likely_elements(const likely_matrix m)
{
    return m->channels * m->columns * m->rows * m->frames;
}

likely_size likely_bytes(const likely_matrix m)
{
    return likely_depth(m->type) * likely_elements(m) / 8;
}

// TODO: make this thread_local when compiler support improves
static likely_matrix recycledBuffer = NULL;

likely_matrix likely_new(likely_type type, likely_size channels, likely_size columns, likely_size rows, likely_size frames, likely_data data, int8_t copy)
{
    likely_matrix m;
    size_t dataBytes = ((data && !copy) ? 0 : uint64_t(likely_depth(type)) * channels * columns * rows * frames / 8);
    const size_t headerBytes = sizeof(likely_matrix_struct) + sizeof(likely_matrix_private);
    if (recycledBuffer) {
        const size_t recycledDataBytes = recycledBuffer->d_ptr->data_bytes;
        if (recycledDataBytes >= dataBytes) { m = recycledBuffer; dataBytes = recycledDataBytes; }
        else                                m = (likely_matrix) realloc(recycledBuffer, headerBytes + dataBytes);
        recycledBuffer = NULL;
    } else {
        m = (likely_matrix) malloc(headerBytes + dataBytes);
    }

    m->type = type;
    m->channels = channels;
    m->columns = columns;
    m->rows = rows;
    m->frames = frames;

    likely_set_multi_channel(&m->type, channels > 1);
    likely_set_multi_column(&m->type, columns > 1);
    likely_set_multi_row(&m->type, rows > 1);
    likely_set_multi_frame(&m->type, frames > 1);

    m->d_ptr = reinterpret_cast<likely_matrix_private*>(m+1);
    m->d_ptr->ref_count = 1;
    m->d_ptr->data_bytes = dataBytes;

    if (data && !copy) {
        m->data = data;
    } else {
        m->data = reinterpret_cast<likely_data>(m->d_ptr+1);
        if (data && copy) memcpy(m->data, data, likely_bytes(m));
    }

    return m;
}

likely_matrix likely_scalar(double value)
{
    likely_matrix m = likely_new(likely_type_from_value(value), 1, 1, 1, 1, NULL, 0);
    likely_set_element(m, value, 0, 0, 0, 0);
    return m;
}

likely_matrix likely_copy(const likely_matrix m, int8_t clone)
{
    return likely_new(m->type, m->channels, m->columns, m->rows, m->frames, m->data, clone);
}

likely_matrix likely_retain(likely_matrix m)
{
    if (m) ++m->d_ptr->ref_count;
    return m;
}

void likely_release(likely_matrix m)
{
    if (!m || --m->d_ptr->ref_count) return;
    if (recycledBuffer) {
        if (m->d_ptr->data_bytes > recycledBuffer->d_ptr->data_bytes)
            swap(m, recycledBuffer);
        free(m);
    } else {
        recycledBuffer = m;
    }
}

double likely_element(const likely_matrix m, likely_size c, likely_size x, likely_size y, likely_size t)
{
    assert(m);
    const likely_size columnStep = m->channels;
    const likely_size rowStep = m->columns * columnStep;
    const likely_size frameStep = m->rows * rowStep;
    const likely_size index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (m->type & likely_type_mask) {
      case likely_type_u8:  return (double)reinterpret_cast< uint8_t*>(m->data)[index];
      case likely_type_u16: return (double)reinterpret_cast<uint16_t*>(m->data)[index];
      case likely_type_u32: return (double)reinterpret_cast<uint32_t*>(m->data)[index];
      case likely_type_u64: return (double)reinterpret_cast<uint64_t*>(m->data)[index];
      case likely_type_i8:  return (double)reinterpret_cast<  int8_t*>(m->data)[index];
      case likely_type_i16: return (double)reinterpret_cast< int16_t*>(m->data)[index];
      case likely_type_i32: return (double)reinterpret_cast< int32_t*>(m->data)[index];
      case likely_type_i64: return (double)reinterpret_cast< int64_t*>(m->data)[index];
      case likely_type_f32: return (double)reinterpret_cast<   float*>(m->data)[index];
      case likely_type_f64: return (double)reinterpret_cast<  double*>(m->data)[index];
      default: assert(false && "likely_element unsupported type");
    }
    return numeric_limits<double>::quiet_NaN();
}

void likely_set_element(likely_matrix m, double value, likely_size c, likely_size x, likely_size y, likely_size t)
{
    assert(m);
    const likely_size columnStep = m->channels;
    const likely_size rowStep = m->columns * columnStep;
    const likely_size frameStep = m->rows * rowStep;
    const likely_size index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (m->type & likely_type_mask) {
      case likely_type_u8:  reinterpret_cast< uint8_t*>(m->data)[index] = ( uint8_t)value; break;
      case likely_type_u16: reinterpret_cast<uint16_t*>(m->data)[index] = (uint16_t)value; break;
      case likely_type_u32: reinterpret_cast<uint32_t*>(m->data)[index] = (uint32_t)value; break;
      case likely_type_u64: reinterpret_cast<uint64_t*>(m->data)[index] = (uint64_t)value; break;
      case likely_type_i8:  reinterpret_cast<  int8_t*>(m->data)[index] = (  int8_t)value; break;
      case likely_type_i16: reinterpret_cast< int16_t*>(m->data)[index] = ( int16_t)value; break;
      case likely_type_i32: reinterpret_cast< int32_t*>(m->data)[index] = ( int32_t)value; break;
      case likely_type_i64: reinterpret_cast< int64_t*>(m->data)[index] = ( int64_t)value; break;
      case likely_type_f32: reinterpret_cast<   float*>(m->data)[index] = (   float)value; break;
      case likely_type_f64: reinterpret_cast<  double*>(m->data)[index] = (  double)value; break;
      default: assert(false && "likely_set_element unsupported type");
    }
}

const char *likely_type_to_string(likely_type type)
{
    static string typeString;

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

    typeString = typeStream.str();
    return typeString.c_str();
}

likely_type likely_type_from_string(const char *str)
{
    assert(str);
    likely_type t = likely_type_null;
    const size_t len = strlen(str);
    if (len == 0) return t;

    if (str[0] == 'f') likely_set_floating(&t, true);
    if (str[0] != 'u') likely_set_signed(&t, true);
    int depth = atoi(str+1); // atoi ignores characters after the number
    if (depth == 0) depth = 32;
    likely_set_depth(&t, depth);

    size_t startIndex = 1;
    while ((startIndex < len) && (str[startIndex] >= '0') && (str[startIndex] <= '9'))
        startIndex++;

    for (size_t i=startIndex; i<len; i++) {
        if      (str[i] == 'P') likely_set_parallel(&t, true);
        else if (str[i] == 'H') likely_set_heterogeneous(&t, true);
        else if (str[i] == 'C') likely_set_multi_channel(&t, true);
        else if (str[i] == 'X') likely_set_multi_column(&t, true);
        else if (str[i] == 'Y') likely_set_multi_row(&t, true);
        else if (str[i] == 'T') likely_set_multi_frame(&t, true);
        else if (str[i] == 'S') likely_set_saturation(&t, true);
        else                    return likely_type_null;
    }

    return t;
}

likely_type likely_type_from_value(double value)
{
    if (ceil(value) == value) {
        if (value >= 0) {
                 if (uint8_t (value) == value) return likely_type_u8;
            else if (uint16_t(value) == value) return likely_type_u16;
            else if (uint32_t(value) == value) return likely_type_u32;
            else if (uint64_t(value) == value) return likely_type_u64;
        } else {
                 if (int8_t (value) == value) return likely_type_i8;
            else if (int16_t(value) == value) return likely_type_i16;
            else if (int32_t(value) == value) return likely_type_i32;
            else if (int64_t(value) == value) return likely_type_i64;
        }
    }

    if (float(value) == value) return likely_type_f32;
    else                       return likely_type_f64;
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
static likely_matrix thunkMatricies[LIKELY_NUM_ARITIES+1];

static void executeWorker(size_t id)
{
    // There are hardware_concurrency-1 helper threads and the main thread with id = 0
    const likely_size step = (thunkSize+numWorkers-1)/numWorkers;
    const likely_size start = id * step;
    const likely_size stop = std::min((id+1)*step, thunkSize);
    if (start >= stop) return;

    // Final three parameters are: dst, start, stop
    typedef void (*likely_kernel_0)(likely_matrix, likely_size, likely_size);
    typedef void (*likely_kernel_1)(const likely_matrix, likely_matrix, likely_size, likely_size);
    typedef void (*likely_kernel_2)(const likely_matrix, const likely_matrix, likely_matrix, likely_size, likely_size);
    typedef void (*likely_kernel_3)(const likely_matrix, const likely_matrix, const likely_matrix, likely_matrix, likely_size, likely_size);

    switch (thunkArity) {
      case 0: reinterpret_cast<likely_kernel_0>(currentThunk)((likely_matrix)thunkMatricies[0], start, stop); break;
      case 1: reinterpret_cast<likely_kernel_1>(currentThunk)(thunkMatricies[0], (likely_matrix)thunkMatricies[1], start, stop); break;
      case 2: reinterpret_cast<likely_kernel_2>(currentThunk)(thunkMatricies[0], thunkMatricies[1], (likely_matrix)thunkMatricies[2], start, stop); break;
      case 3: reinterpret_cast<likely_kernel_3>(currentThunk)(thunkMatricies[0], thunkMatricies[1], thunkMatricies[2], (likely_matrix)thunkMatricies[3], start, stop); break;
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

extern "C" LIKELY_EXPORT void likely_fork(void *thunk, likely_arity arity, likely_size size, likely_matrix src, ...)
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
        src = va_arg(ap, likely_matrix);
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
