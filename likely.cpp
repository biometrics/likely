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

#include <llvm/PassManager.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetLibraryInfo.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Vectorize.h>
#include <lua.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdlib.h>
#include <atomic>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <regex>
#include <stack>
#include <sstream>
#include <thread>

#include "likely.h"
#include "opencv.shim"

using namespace llvm;
using namespace std;

#define LIKELY_NUM_ARITIES 4

#define LLVM_VALUE_TO_INT(VALUE) (llvm::cast<Constant>(VALUE)->getUniqueInteger().getZExtValue())

static StructType *TheMatrixStruct = NULL;

typedef struct likely_matrix_private
{
    size_t data_bytes;
    int ref_count;
} likely_matrix_private;

static bool likelyAssertHelper(bool condition, const char *format, va_list ap, lua_State *L = NULL)
{
    if (condition) return true;
    static const int errorBufferSize = 1024;
    static char errorBuffer[errorBufferSize];
    vsnprintf(errorBuffer, errorBufferSize, format, ap);
    if (L) luaL_error(L, "Likely %s.", errorBuffer);
    else   fprintf(stderr, "Likely %s.\n", errorBuffer);
    return false;
}

bool likely_assert(bool condition, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    return likelyAssertHelper(condition, format, ap);
}

static bool lua_likely_assert(lua_State *L, bool condition, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    return likelyAssertHelper(condition, format, ap, L);
}

static likely_mat checkLuaMat(lua_State *L, int index = 1)
{
    likely_mat *mp = (likely_mat*)luaL_checkudata(L, index, "likely");
    if (!mp) return NULL;
    return *mp;
}

static int lua_likely__tostring(lua_State *L)
{
    likely_assert(lua_gettop(L) == 1, "'__tostring' expected 1 argument, got: %d", lua_gettop(L));
    likely_const_mat m = checkLuaMat(L);
    lua_pushfstring(L, "Likely %dx%dx%dx%d %s %p", m->channels, m->columns, m->rows, m->frames, likely_type_to_string(m->type), m);
    return 1;
}

static int likely_get(likely_type type, likely_type_field mask) { return type & mask; }
static void likely_set(likely_type *type, int i, likely_type_field mask) { *type &= ~mask; *type |= i & mask; }
static bool likely_get_bool(likely_type type, likely_type_field mask) { return type & mask; }
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

static int lua_likely_get(lua_State *L)
{
    likely_assert(lua_gettop(L) == 2, "'get' expected 2 arguments, got: %d", lua_gettop(L));
    likely_const_mat m = checkLuaMat(L);
    const char *field = lua_tostring(L, 2);
    if      (!strcmp(field, "likely"))        lua_pushstring(L, "matrix");
    else if (!strcmp(field, "data"))          lua_pushlightuserdata(L, m->data);
    else if (!strcmp(field, "type"))          lua_pushinteger(L, m->type);
    else if (!strcmp(field, "channels"))      lua_pushinteger(L, m->channels);
    else if (!strcmp(field, "columns"))       lua_pushinteger(L, m->columns);
    else if (!strcmp(field, "rows"))          lua_pushinteger(L, m->rows);
    else if (!strcmp(field, "frames"))        lua_pushinteger(L, m->frames);
    else if (!strcmp(field, "depth"))         lua_pushinteger(L, likely_depth(m->type));
    else if (!strcmp(field, "signed"))        lua_pushboolean(L, likely_signed(m->type));
    else if (!strcmp(field, "floating"))      lua_pushboolean(L, likely_floating(m->type));
    else if (!strcmp(field, "parallel"))      lua_pushboolean(L, likely_parallel(m->type));
    else if (!strcmp(field, "heterogeneous")) lua_pushboolean(L, likely_heterogeneous(m->type));
    else if (!strcmp(field, "multiChannel"))  lua_pushboolean(L, likely_multi_channel(m->type));
    else if (!strcmp(field, "multiColumn"))   lua_pushboolean(L, likely_multi_column(m->type));
    else if (!strcmp(field, "multiRow"))      lua_pushboolean(L, likely_multi_row(m->type));
    else if (!strcmp(field, "multiFrame"))    lua_pushboolean(L, likely_multi_frame(m->type));
    else if (!strcmp(field, "saturation"))    lua_pushboolean(L, likely_saturation(m->type));
    else if (!strcmp(field, "reserved"))      lua_pushinteger(L, likely_reserved(m->type));
    else                                      lua_pushnil(L);
    return 1;
}

static int lua_likely__index(lua_State *L)
{
    likely_assert(lua_gettop(L) == 2, "'__index' expected 2 arguments, got: %d", lua_gettop(L));
    const char *key = luaL_checkstring(L, 2);
    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);

    // Either key is name of a method in the metatable
    if (!lua_isnil(L, -1))
        return 1;

    // ... or its a field access, so recall as self.get(self, value).
    lua_settop(L, 2);
    return lua_likely_get(L);
}

static int lua_likely_set(lua_State *L)
{
    likely_assert(lua_gettop(L) == 3, "'set' expected 3 arguments, got: %d", lua_gettop(L));
    likely_mat m = checkLuaMat(L);
    const char *field = lua_tostring(L, 2);
    int isnum;
    if      (!strcmp(field, "data"))          m->data = (likely_data*) lua_touserdata(L, 3);
    else if (!strcmp(field, "type"))        { m->type  = lua_tointegerx(L, 3, &isnum); likely_assert(isnum, "'set' expected type to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "channels"))    { m->channels  = lua_tointegerx(L, 3, &isnum); likely_assert(isnum, "'set' expected channels to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "columns"))     { m->columns   = lua_tointegerx(L, 3, &isnum); likely_assert(isnum, "'set' expected columns to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "rows"))        { m->rows      = lua_tointegerx(L, 3, &isnum); likely_assert(isnum, "'set' expected rows to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "frames"))      { m->frames    = lua_tointegerx(L, 3, &isnum); likely_assert(isnum, "'set' expected frames to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "depth"))       { likely_set_depth(&m->type, lua_tointegerx(L, 3, &isnum)); likely_assert(isnum, "'set' expected depth to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "signed"))        likely_set_signed(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "floating"))      likely_set_floating(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "parallel"))      likely_set_parallel(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "heterogeneous")) likely_set_heterogeneous(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "multiChannel"))  likely_set_multi_channel(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "multiColumn"))   likely_set_multi_column(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "multiRow"))      likely_set_multi_row(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "multiFrame"))    likely_set_multi_frame(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "saturation"))    likely_set_saturation(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "reserved"))    { likely_set_reserved(&m->type, lua_tointegerx(L, 3, &isnum)); likely_assert(isnum, "'set' expected reserved to be an integer, got: %s", lua_tostring(L, 3)); }
    else                                      likely_assert(false, "unrecognized field: %s", field);
    return 0;
}

static int lua_likely__newindex(lua_State *L)
{
    likely_assert(lua_gettop(L) == 3, "'__newindex' expected 3 arguments, got: %d", lua_gettop(L));
    return lua_likely_set(L);
}

likely_size likely_elements(likely_const_mat m)
{
    return m->channels * m->columns * m->rows * m->frames;
}

static int lua_likely_elements(lua_State *L)
{
    likely_assert(lua_gettop(L) == 1, "'elements' expected 1 argument, got: %d", lua_gettop(L));
    lua_pushinteger(L, likely_elements(checkLuaMat(L)));
    return 1;
}

likely_size likely_bytes(likely_const_mat m)
{
    return uint64_t(likely_depth(m->type)) * uint64_t(likely_elements(m)) / uint64_t(8);
}

static int lua_likely_bytes(lua_State *L)
{
    likely_assert(lua_gettop(L) == 1, "'bytes' expected 1 argument, got: %d", lua_gettop(L));
    lua_pushinteger(L, likely_bytes(checkLuaMat(L)));
    return 1;
}

static likely_mat *newLuaMat(lua_State *L)
{
    likely_mat *mp = (likely_mat*) lua_newuserdata(L, sizeof(likely_mat));
    luaL_getmetatable(L, "likely");
    lua_setmetatable(L, -2);
    return mp;
}

// TODO: make this thread_local when compiler support improves
static likely_mat recycledBuffer = NULL;

static likely_data *dataPointer(likely_mat m)
{
    return reinterpret_cast<likely_data*>(uintptr_t(m+1) + sizeof(likely_matrix_private));
}

likely_mat likely_new(likely_type type, likely_size channels, likely_size columns, likely_size rows, likely_size frames, likely_data *data, int8_t copy)
{
    likely_mat m;
    size_t dataBytes = ((data && !copy) ? 0 : uint64_t(likely_depth(type)) * channels * columns * rows * frames / 8);
    const size_t headerBytes = sizeof(likely_matrix) + sizeof(likely_matrix_private);
    if (recycledBuffer) {
        const size_t recycledDataBytes = recycledBuffer->d_ptr->data_bytes;
        if (recycledDataBytes >= dataBytes) { m = recycledBuffer; dataBytes = recycledDataBytes; }
        else                                m = (likely_mat) realloc(recycledBuffer, headerBytes + dataBytes);
        recycledBuffer = NULL;
    } else {
        m = (likely_mat) malloc(headerBytes + dataBytes);
    }

    m->type = type;
    m->channels = channels;
    m->columns = columns;
    m->rows = rows;
    m->frames = frames;

    likely_set_multi_channel(&m->type, channels != 1);
    likely_set_multi_column(&m->type, columns != 1);
    likely_set_multi_row(&m->type, rows != 1);
    likely_set_multi_frame(&m->type, frames != 1);

    assert(alignof(likely_matrix_private) <= alignof(likely_matrix));
    m->d_ptr = reinterpret_cast<likely_matrix_private*>(m+1);
    m->d_ptr->ref_count = 1;
    m->d_ptr->data_bytes = dataBytes;

    if (data && !copy) {
        m->data = data;
    } else {
        m->data = dataPointer(m);
        if (data && copy) memcpy(m->data, data, likely_bytes(m));
    }

    return m;
}

static int lua_likely_new(lua_State *L)
{
    likely_type type = likely_type_f32;
    likely_size channels = 1;
    likely_size columns = 1;
    likely_size rows = 1;
    likely_size frames = 1;
    likely_data *data = NULL;
    int8_t copy = 0;

    int isnum;
    const int argc = lua_gettop(L);
    switch (argc) {
      case 7: copy     = lua_toboolean(L, 7);
      case 6: data     = (likely_data*) lua_touserdata(L, 6);
      case 5: frames   = lua_tointegerx(L, 5, &isnum); likely_assert(isnum, "'new' expected frames to be an integer, got: %s", lua_tostring(L, 5));
      case 4: rows     = lua_tointegerx(L, 4, &isnum); likely_assert(isnum, "'new' expected rows to be an integer, got: %s", lua_tostring(L, 4));
      case 3: columns  = lua_tointegerx(L, 3, &isnum); likely_assert(isnum, "'new' expected columns to be an integer, got: %s", lua_tostring(L, 3));
      case 2: channels = lua_tointegerx(L, 2, &isnum); likely_assert(isnum, "'new' expected channels to be an integer, got: %s", lua_tostring(L, 2));
      case 1: type     = lua_tointegerx(L, 1, &isnum); likely_assert(isnum, "'new' expected type to be an integer, got: %s", lua_tostring(L, 2));
      case 0: break;
      default: likely_assert(false, "'new' expected no more than 7 arguments, got: %d", argc);
    }

    *newLuaMat(L) = likely_new(type, channels, columns, rows, frames, data, copy);
    return 1;
}

likely_mat likely_scalar(double value)
{
    likely_mat m = likely_new(likely_type_from_value(value));
    likely_set_element(m, value);
    return m;
}

static int lua_likely_scalar(lua_State *L)
{
    likely_assert(lua_gettop(L) == 1, "'scalar' expected 1 argument, got: %d", lua_gettop(L));
    int isnum;
    double value = lua_tonumberx(L, 1, &isnum);
    likely_assert(isnum, "'scalar' expected a numeric argument, got: %s", lua_tostring(L, 1));
    *newLuaMat(L) = likely_scalar(value);
    return 1;
}

likely_mat likely_copy(likely_const_mat m, int8_t copy_data)
{
    return likely_new(m->type, m->channels, m->columns, m->rows, m->frames, m->data, copy_data);
}

static int lua_likely_copy(lua_State *L)
{
    const int args = lua_gettop(L);
    bool copy_data = false;
    likely_mat m = NULL;
    switch (args) {
        case 2: copy_data = lua_toboolean(L, 2);
        case 1: m = checkLuaMat(L);
        case 0: break;
        default: lua_likely_assert(L, false, "'copy' expected 1-2 arguments, got: %d", args);
    }
    *newLuaMat(L) = likely_copy(m, copy_data);
    return 1;
}

likely_mat likely_retain(likely_mat m)
{
    if (!m) return m;
    ++m->d_ptr->ref_count;
    return m;
}

void likely_release(likely_mat m)
{
    if (!m) return;
    if (--m->d_ptr->ref_count != 0) return;
    const size_t dataBytes = m->d_ptr->data_bytes;
    if (recycledBuffer) {
        if (dataBytes > recycledBuffer->d_ptr->data_bytes)
            swap(m, recycledBuffer);
        free(m);
    } else {
        recycledBuffer = m;
    }
}

static int lua_likely__gc(lua_State *L)
{
    likely_assert(lua_gettop(L) == 1, "'__gc' expected 1 argument, got: %d", lua_gettop(L));
    likely_release(checkLuaMat(L));
    return 0;
}

static likely_mat likelyReadHelper(const char *fileName, lua_State *L = NULL)
{
    static string previousFileName;
    static likely_mat previousMat = NULL;
    if (previousFileName == fileName)
        return likely_copy(previousMat);

    cv::Mat m = cv::imread(fileName, CV_LOAD_IMAGE_UNCHANGED);
    lua_likely_assert(L, m.data, "'read' failed to open: %s", fileName);
    likely_mat mat = fromCvMat(m, true);

    likely_release(previousMat);
    likely_retain(mat);
    previousMat = mat;
    previousFileName = fileName;
    return mat;
}

likely_mat likely_read(const char *file_name)
{
    return likelyReadHelper(file_name);
}

static int lua_likely_read(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 1, "'read' expected 1 argument, got: %d", lua_gettop(L));
    *newLuaMat(L) = likelyReadHelper(lua_tostring(L, 1), L);
    return 1;
}

void likely_write(likely_const_mat image, const char *file_name)
{
    cv::imwrite(file_name, toCvMat(image));
}

static int lua_likely_write(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 2, "'write' expected 2 arguments, got: %d", lua_gettop(L));
    likely_write(checkLuaMat(L), lua_tostring(L, 2));
    return 0;
}

likely_mat likely_decode(likely_const_mat buffer)
{
    return fromCvMat(cv::imdecode(toCvMat(buffer), CV_LOAD_IMAGE_UNCHANGED), true);
}

static int lua_likely_decode(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 1, "'decode' expected 1 argument, got: %d", lua_gettop(L));
    *newLuaMat(L) = likely_decode(checkLuaMat(L));
    return 1;
}

likely_mat likely_encode(likely_const_mat image, const char *extension)
{
    vector<uchar> buf;
    cv::imencode(extension, toCvMat(image), buf);
    return fromCvMat(cv::Mat(buf), true);
}

static int lua_likely_encode(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 2, "'write' expected 2 arguments, got: %d", lua_gettop(L));
    *newLuaMat(L) = likely_encode(checkLuaMat(L), lua_tostring(L, 2));
    return 1;
}

likely_mat likely_render(likely_const_mat m, double *min_, double *max_)
{
    double min, max, range;
    if ((m->type & likely_type_mask) != likely_type_u8) {
        min = std::numeric_limits<double>::max();
        max = -std::numeric_limits<double>::max();
        for (likely_size t=0; t<m->frames; t++) {
            for (likely_size y=0; y<m->rows; y++) {
                for (likely_size x=0; x<m->columns; x++) {
                    for (likely_size c=0; c<m->channels; c++) {
                        const double value = likely_element(m, c, x, y, t);
                        min = std::min(min, value);
                        max = std::max(max, value);
                    }
                }
            }
        }
        range = (max - min)/255;
        if ((range >= 0.25) && (range < 1)) {
            max = min + 255;
            range = 1;
        }
    } else {
        min = 0;
        max = 255;
        range = 1;
        // Special case, return the original image
        if (m->channels == 3) {
            if (min_) *min_ = min;
            if (max_) *max_ = max;
            return likely_copy(m);
        }
    }

    likely_mat n = likely_new(likely_type_u8, 3, m->columns, m->rows);
    for (likely_size y=0; y<n->rows; y++) {
        for (likely_size x=0; x<n->columns; x++) {
            for (likely_size c=0; c<3; c++) {
                const double value = likely_element(m, c % m->channels, x, y);
                likely_set_element(n, (value-min)/range, c, x, y);
            }
        }
    }

    if (min_) *min_ = min;
    if (max_) *max_ = max;
    return n;
}

static int lua_likely_render(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 1, "'render' expected 1 argument, got: %d", lua_gettop(L));
    *newLuaMat(L) = likely_render(checkLuaMat(L));
    return 1;
}

double likely_element(likely_const_mat m, likely_size c, likely_size x, likely_size y, likely_size t)
{
    likely_assert(m, "likely_element received a null matrix");
    const likely_size columnStep = m->channels;
    const likely_size rowStep = m->columns * columnStep;
    const likely_size frameStep = m->rows * rowStep;
    const likely_size index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (m->type & likely_type_mask) {
      case likely_type_u8:  return reinterpret_cast< uint8_t*>(m->data)[index];
      case likely_type_u16: return reinterpret_cast<uint16_t*>(m->data)[index];
      case likely_type_u32: return reinterpret_cast<uint32_t*>(m->data)[index];
      case likely_type_u64: return reinterpret_cast<uint64_t*>(m->data)[index];
      case likely_type_i8:  return reinterpret_cast<  int8_t*>(m->data)[index];
      case likely_type_i16: return reinterpret_cast< int16_t*>(m->data)[index];
      case likely_type_i32: return reinterpret_cast< int32_t*>(m->data)[index];
      case likely_type_i64: return reinterpret_cast< int64_t*>(m->data)[index];
      case likely_type_f32: return reinterpret_cast<   float*>(m->data)[index];
      case likely_type_f64: return reinterpret_cast<  double*>(m->data)[index];
      default: likely_assert(false, "likely_element unsupported type");
    }
    return numeric_limits<double>::quiet_NaN();
}

void likely_set_element(likely_mat m, double value, likely_size c, likely_size x, likely_size y, likely_size t)
{
    likely_assert(m, "likely_set_element received a null matrix");
    const likely_size columnStep = m->channels;
    const likely_size rowStep = m->columns * columnStep;
    const likely_size frameStep = m->rows * rowStep;
    const likely_size index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (m->type & likely_type_mask) {
      case likely_type_u8:  reinterpret_cast< uint8_t*>(m->data)[index] = value; break;
      case likely_type_u16: reinterpret_cast<uint16_t*>(m->data)[index] = value; break;
      case likely_type_u32: reinterpret_cast<uint32_t*>(m->data)[index] = value; break;
      case likely_type_u64: reinterpret_cast<uint64_t*>(m->data)[index] = value; break;
      case likely_type_i8:  reinterpret_cast<  int8_t*>(m->data)[index] = value; break;
      case likely_type_i16: reinterpret_cast< int16_t*>(m->data)[index] = value; break;
      case likely_type_i32: reinterpret_cast< int32_t*>(m->data)[index] = value; break;
      case likely_type_i64: reinterpret_cast< int64_t*>(m->data)[index] = value; break;
      case likely_type_f32: reinterpret_cast<   float*>(m->data)[index] = value; break;
      case likely_type_f64: reinterpret_cast<  double*>(m->data)[index] = value; break;
      default: likely_assert(false, "likely_set_element unsupported type");
    }
}

const char *likely_type_to_string(likely_type type)
{
    static string typeString; // Provides return value persistence

    stringstream typeStream;
    typeStream << (likely_floating(type) ? "f" : (likely_signed(type) ? "i" : "u"));
    typeStream << likely_depth(type);

    if (likely_parallel(type))       typeStream << "P";
    if (likely_heterogeneous(type))  typeStream << "H";
    if (likely_multi_channel(type)) typeStream << "C";
    if (likely_multi_column(type))  typeStream << "X";
    if (likely_multi_row(type))     typeStream << "Y";
    if (likely_multi_frame(type))   typeStream << "T";
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
        if (str[i] == 'P') likely_set_parallel(&t, true);
        if (str[i] == 'H') likely_set_heterogeneous(&t, true);
        if (str[i] == 'C') likely_set_multi_channel(&t, true);
        if (str[i] == 'X') likely_set_multi_column(&t, true);
        if (str[i] == 'Y') likely_set_multi_row(&t, true);
        if (str[i] == 'T') likely_set_multi_frame(&t, true);
        if (str[i] == 'S') likely_set_saturation(&t, true);
    }

    return t;
}

likely_type likely_type_from_value(double value)
{
    if      (static_cast< uint8_t>(value) == value) return likely_type_u8;
    else if (static_cast<  int8_t>(value) == value) return likely_type_i8;
    else if (static_cast<uint16_t>(value) == value) return likely_type_u16;
    else if (static_cast< int16_t>(value) == value) return likely_type_i16;
    else if (static_cast<uint32_t>(value) == value) return likely_type_u32;
    else if (static_cast< int32_t>(value) == value) return likely_type_i32;
    else if (static_cast<uint64_t>(value) == value) return likely_type_u64;
    else if (static_cast< int64_t>(value) == value) return likely_type_i64;
    else if (static_cast<   float>(value) == value) return likely_type_f32;
    else                                            return likely_type_f64;
}

likely_type likely_type_from_types(likely_type lhs, likely_type rhs)
{
    likely_type result = lhs | rhs;
    likely_set_depth(&result, max(likely_depth(lhs), likely_depth(rhs)));
    return result;
}

void likely_print(likely_const_mat m)
{
    if (!m) return;
    for (uint t=0; t<m->frames; t++) {
        for (uint y=0; y<m->rows; y++) {
            cout << (m->rows > 1 ? (y == 0 ? "[" : " ") : "");
            for (uint x=0; x<m->columns; x++) {
                for (uint c=0; c<m->channels; c++) {
                    cout << likely_element(m, c, x, y, t);
                    if (c != m->channels-1)
                        cout << " ";
                }
                cout << (m->channels > 1 ? ";" : (x < m->columns-1 ? " " : ""));
            }
            cout << ((m->columns > 1) && (y < m->rows-1) ? "\n" : "");
        }
        cout << (m->rows > 1 ? "]\n" : "");
        cout << (t < m->frames-1 ? "\n" : "");
    }
}

struct TypedValue
{
    Value *value;
    likely_type type;
    TypedValue() : value(NULL), type(likely_type_null) {}
    TypedValue(Value *value_, likely_type type_) : value(value_), type(type_) {}
    operator Value*() const { return value; }
    operator likely_type() const { return type; }
};

struct KernelBuilder
{
    Module *m;
    IRBuilder<> *b;
    Function *f;

    struct Loop {
        BasicBlock *body;
        PHINode *i;
        Value *stop;
        MDNode *node;
    };
    stack<Loop> loops;

    KernelBuilder(Module *module, IRBuilder<> *builder, Function *function)
        : m(module), b(builder), f(function)
    {}

    static TypedValue constant(double value, likely_type type)
    {
        const int depth = likely_depth(type);
        if (likely_floating(type)) {
            if (value == 0) value = -0; // IEEE/LLVM optimization quirk
            if      (depth == 64) return TypedValue(ConstantFP::get(Type::getDoubleTy(getGlobalContext()), value), type);
            else if (depth == 32) return TypedValue(ConstantFP::get(Type::getFloatTy(getGlobalContext()), value), type);
            else                  { likely_assert(false, "invalid floating point constant depth: %d", depth); return TypedValue(); }
        } else {
            return TypedValue(Constant::getIntegerValue(Type::getIntNTy(getGlobalContext(), depth), APInt(depth, value)), type);
        }
    }

    static Constant *constant(int value, int bits = 32) { return Constant::getIntegerValue(Type::getInt32Ty(getGlobalContext()), APInt(bits, value)); }
    static Constant *constant(bool value) { return constant(value, 1); }
    static Constant *constant(float value) { return ConstantFP::get(Type::getFloatTy(getGlobalContext()), value == 0 ? -0.0f : value); }
    static Constant *constant(double value) { return ConstantFP::get(Type::getDoubleTy(getGlobalContext()), value == 0 ? -0.0 : value); }
    static Constant *constant(const char *value) { return ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(getGlobalContext(), 8*sizeof(value)), uint64_t(value)), Type::getInt8PtrTy(getGlobalContext())); }
    template <typename T>
    static Constant *constant(T value, Type *type) { return ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(getGlobalContext(), 8*sizeof(value)), uint64_t(value)), type); }
    static Constant *zero(int bits = 32) { return constant(0, bits); }
    static Constant *one(int bits = 32) { return constant(1, bits); }
    static Constant *intMax(int bits = 32) { return constant((1 << (bits-1))-1, bits); }
    static Constant *intMin(int bits = 32) { return constant((1 << (bits-1)), bits); }

    Value *data(const TypedValue &matrix) const { return b->CreatePointerCast(b->CreateLoad(b->CreateStructGEP(matrix, 0), "data"), ty(matrix, true)); }
    Value *type(Value *v) const { return b->CreateLoad(b->CreateStructGEP(v, 1), "type"); }
    Value *type(likely_type type) const { return constant(type, 32); }
    Value *channels(Value *v) const { return b->CreateLoad(b->CreateStructGEP(v, 2), "channels"); }
    Value *columns(Value *v) const { return b->CreateLoad(b->CreateStructGEP(v, 3), "columns"); }
    Value *rows(Value *v) const { return b->CreateLoad(b->CreateStructGEP(v, 4), "rows"); }
    Value *frames(Value *v) const { return b->CreateLoad(b->CreateStructGEP(v, 5), "frames"); }

    void setData(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 0)); }
    void setType(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 1)); }
    void setChannels(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 2)); }
    void setColumns(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 3)); }
    void setRows(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 4)); }
    void setFrames(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 5)); }

    Value *get(Value *matrix, int mask) const { return b->CreateAnd(type(matrix), constant(mask, 8*sizeof(likely_type))); }
    void set(Value *matrix, int value, int mask) const { setType(matrix, b->CreateOr(b->CreateAnd(type(matrix), constant(~mask, 8*sizeof(likely_type))), b->CreateAnd(constant(value, 8*sizeof(likely_type)), constant(mask, 8*sizeof(likely_type))))); }
    void setBit(Value *matrix, bool on, int mask) const { on ? setType(matrix, b->CreateOr(type(matrix), constant(mask, 8*sizeof(likely_type)))) : setType(matrix, b->CreateAnd(type(matrix), constant(~mask, 8*sizeof(likely_type)))); }

    Value *depth(Value *matrix) const { return get(matrix, likely_type_depth); }
    void setDepth(Value *matrix, int depth) const { set(matrix, depth, likely_type_depth); }
    Value *signed_(Value *matrix) const { return get(matrix, likely_type_signed); }
    void setSigned(Value *matrix, bool signed_) const { setBit(matrix, signed_, likely_type_signed); }
    Value *floating(Value *matrix) const { return get(matrix, likely_type_floating); }
    void setFloating(Value *matrix, bool floating) const { setBit(matrix, floating, likely_type_floating); }
    Value *parallel(Value *matrix) const { return get(matrix, likely_type_parallel); }
    void setParallel(Value *matrix, bool parallel) const { setBit(matrix, parallel, likely_type_parallel); }
    Value *heterogeneous(Value *matrix) const { return get(matrix, likely_type_heterogeneous); }
    void setHeterogeneous(Value *matrix, bool heterogeneous) const { setBit(matrix, heterogeneous, likely_type_heterogeneous); }
    Value *multiChannel(Value *matrix) const { return get(matrix, likely_type_multi_channel); }
    void setMultiChannel(Value *matrix, bool multiChannel) const { setBit(matrix, multiChannel, likely_type_multi_channel); }
    Value *multiColumn(Value *matrix) const { return get(matrix, likely_type_multi_column); }
    void setMultiColumn(Value *matrix, bool multiColumn) { setBit(matrix, multiColumn, likely_type_multi_column); }
    Value *multiRow(Value *matrix) const { return get(matrix, likely_type_multi_row); }
    void setMultiRow(Value *matrix, bool multiRow) const { setBit(matrix, multiRow, likely_type_multi_row); }
    Value *multiFrame(Value *matrix) const { return get(matrix, likely_type_multi_frame); }
    void multiFrame(Value *matrix, bool multiFrame) const { setBit(matrix, multiFrame, likely_type_multi_frame); }
    Value *saturation(Value *matrix) const { return get(matrix, likely_type_saturation); }
    void setSaturation(Value *matrix, bool saturation) const { setBit(matrix, saturation, likely_type_saturation); }
    Value *reserved(Value *matrix) const { return get(matrix, likely_type_reserved); }
    void setReserved(Value *matrix, int reserved) const { set(matrix, reserved, likely_type_reserved); }

    Value *elements(Value *matrix) const { return b->CreateMul(b->CreateMul(b->CreateMul(channels(matrix), columns(matrix)), rows(matrix)), frames(matrix)); }
    Value *bytes(Value *matrix) const { return b->CreateMul(b->CreateUDiv(b->CreateCast(Instruction::ZExt, depth(matrix), Type::getInt32Ty(getGlobalContext())), constant(8, 32)), elements(matrix)); }

    Value *columnStep(Value *matrix) const { Value *columnStep = channels(matrix); columnStep->setName("cStep"); return columnStep; }
    Value *rowStep(Value *matrix) const { return b->CreateMul(columns(matrix), columnStep(matrix), "rStep"); }
    Value *frameStep(Value *matrix) const { return b->CreateMul(rows(matrix), rowStep(matrix), "tStep"); }

    Value *index(const TypedValue &matrix, Value *c) const
    {
        if (likely_multi_channel(matrix)) return c;
        else                              return zero();
    }

    Value *index(const TypedValue &matrix, Value *c, Value *x) const
    {
        Value *remainder = index(matrix, c);
        if (likely_multi_column(matrix)) return b->CreateAdd(b->CreateMul(x, columnStep(matrix)), remainder);
        else                             return remainder;
    }

    Value *index(const TypedValue &matrix, Value *c, Value *x, Value *y) const
    {
        Value *remainder = index(matrix, c, x);
        if (likely_multi_row(matrix)) return b->CreateAdd(b->CreateMul(y, rowStep(matrix)), remainder);
        else                          return remainder;
    }

    Value *index(const TypedValue &matrix, Value *c, Value *x, Value *y, Value *t) const
    {
        Value *remainder = index(matrix, c, x, y);
        if (likely_multi_frame(matrix)) return b->CreateAdd(b->CreateMul(t, frameStep(matrix)), remainder);
        else                            return remainder;
    }

    void deindex(const TypedValue &matrix, Value *i, Value **c) const
    {
        if (likely_multi_channel(matrix)) *c = i;
        else                              *c = zero();
    }

    void deindex(const TypedValue &matrix, Value *i, Value **c, Value **x) const
    {
        Value *remainder;
        if (likely_multi_column(matrix)) {
            Value *step = columnStep(matrix);
            remainder = b->CreateURem(i, step, "xRem");
            *x = b->CreateExactUDiv(b->CreateSub(i, remainder), step, "x");
        } else {
            remainder = i;
            *x = zero();
        }
        deindex(matrix, remainder, c);
    }

    void deindex(const TypedValue &matrix, Value *i, Value **c, Value **x, Value **y) const
    {
        Value *remainder;
        if (likely_multi_row(matrix)) {
            Value *step = rowStep(matrix);
            remainder = b->CreateURem(i, step, "yRem");
            *y = b->CreateExactUDiv(b->CreateSub(i, remainder), step, "y");
        } else {
            remainder = i;
            *y = zero();
        }
        deindex(matrix, remainder, c, x);
    }

    void deindex(const TypedValue &matrix, Value *i, Value **c, Value **x, Value **y, Value **t) const
    {
        Value *remainder;
        if (likely_multi_frame(matrix)) {
            Value *step = frameStep(matrix);
            remainder = b->CreateURem(i, step, "tRem");
            *t = b->CreateExactUDiv(b->CreateSub(i, remainder), step, "t");
        } else {
            remainder = i;
            *t = zero();
        }
        deindex(matrix, remainder, c, x, y);
    }

    TypedValue GEP(const TypedValue &matrix, Value *i)
    {
        return TypedValue(b->CreateGEP(data(matrix), i), matrix.type);
    }

    TypedValue load(const TypedValue &matrix, Value *i)
    {
        LoadInst *load = b->CreateLoad(GEP(matrix, i));
        load->setMetadata("llvm.mem.parallel_loop_access", getCurrentNode());
        return TypedValue(load, matrix.type);
    }

    TypedValue store(const TypedValue &matrix, Value *i, const TypedValue &value)
    {
        StoreInst *store = b->CreateStore(value, GEP(matrix, i));
        store->setMetadata("llvm.mem.parallel_loop_access", getCurrentNode());
        return TypedValue(store, matrix.type);
    }

    TypedValue cast(const TypedValue &x, likely_type type) const
    {
        if ((x.type & likely_type_mask) == (type & likely_type_mask))
            return x;
        Type *dstType = ty(type);
        return TypedValue(b->CreateCast(CastInst::getCastOpcode(x, likely_signed(x.type), dstType, likely_signed(type)), x, dstType), type);
    }

    static likely_type validFloatType(likely_type type)
    {
        likely_set_floating(&type, true);
        likely_set_signed(&type, true);
        likely_set_depth(&type, likely_depth(type) > 32 ? 64 : 32);
        return type;
    }

    Loop beginLoop(BasicBlock *entry, Value *start, Value *stop)
    {
        Loop loop;
        loop.stop = stop;
        loop.body = BasicBlock::Create(getGlobalContext(), "loop_body", f);

        // Create self-referencing loop node
        vector<Value*> metadata;
        MDNode *tmp = MDNode::getTemporary(getGlobalContext(), metadata);
        metadata.push_back(tmp);
        loop.node = MDNode::get(getGlobalContext(), metadata);
        tmp->replaceAllUsesWith(loop.node);
        MDNode::deleteTemporary(tmp);

        // Loops assume there is at least one iteration
        b->CreateBr(loop.body);
        b->SetInsertPoint(loop.body);

        loop.i = b->CreatePHI(Type::getInt32Ty(getGlobalContext()), 2, "i");
        loop.i->addIncoming(start, entry);

        loops.push(loop);
        return loop;
    }

    void endLoop()
    {
        const Loop &loop = loops.top();
        Value *increment = b->CreateAdd(loop.i, one(), "loop_increment");
        BasicBlock *loopLatch = BasicBlock::Create(getGlobalContext(), "loop_latch", f);
        b->CreateBr(loopLatch);
        b->SetInsertPoint(loopLatch);
        BasicBlock *loopExit = BasicBlock::Create(getGlobalContext(), "loop_exit", f);
        BranchInst *latch = b->CreateCondBr(b->CreateICmpEQ(increment, loop.stop, "loop_test"), loopExit, loop.body);
        latch->setMetadata("llvm.loop", loop.node);
        loop.i->addIncoming(increment, loopLatch);
        b->SetInsertPoint(loopExit);
        loops.pop();
    }

    MDNode *getCurrentNode() const { return loops.empty() ? NULL : loops.top().node; }

    template <typename T>
    inline static vector<T> toVector(T value) { vector<T> vector; vector.push_back(value); return vector; }

    static Type *ty(likely_type type, bool pointer = false)
    {
        const int bits = likely_depth(type);
        const bool floating = likely_floating(type);
        if (floating) {
            if      (bits == 16) return pointer ? Type::getHalfPtrTy(getGlobalContext())   : Type::getHalfTy(getGlobalContext());
            else if (bits == 32) return pointer ? Type::getFloatPtrTy(getGlobalContext())  : Type::getFloatTy(getGlobalContext());
            else if (bits == 64) return pointer ? Type::getDoublePtrTy(getGlobalContext()) : Type::getDoubleTy(getGlobalContext());
        } else {
            if      (bits == 1)  return pointer ? Type::getInt1PtrTy(getGlobalContext())  : (Type*)Type::getInt1Ty(getGlobalContext());
            else if (bits == 8)  return pointer ? Type::getInt8PtrTy(getGlobalContext())  : (Type*)Type::getInt8Ty(getGlobalContext());
            else if (bits == 16) return pointer ? Type::getInt16PtrTy(getGlobalContext()) : (Type*)Type::getInt16Ty(getGlobalContext());
            else if (bits == 32) return pointer ? Type::getInt32PtrTy(getGlobalContext()) : (Type*)Type::getInt32Ty(getGlobalContext());
            else if (bits == 64) return pointer ? Type::getInt64PtrTy(getGlobalContext()) : (Type*)Type::getInt64Ty(getGlobalContext());
        }
        likely_assert(false, "MatrixBuilder::ty invalid matrix bits: %d and floating: %d", bits, floating);
        return NULL;
    }
};

struct SExp
{
    string op;
    vector<SExp> sexps;
    SExp() = default;
    SExp(const string &source)
    {
        likely_assert(!source.empty(), "empty expression");
        if ((source[0] == '(') && (source[source.size()-1] == ')')) {
            size_t start = 1, depth = 0;
            for (size_t i=1; i<source.size()-1; i++) {
                if (source[i] == '(') {
                    depth++;
                } else if (source[i] == ')') {
                    likely_assert(depth > 0, "unexpected ')' in: %s", source.c_str());
                    depth--;
                } else if ((source[i] == ' ') && (depth == 0)) {
                    if (start != i)
                        sexps.push_back(source.substr(start, i-start));
                    start = i + 1;
                }
            }
            likely_assert(depth == 0, "missing ')' in :%s", source.c_str());
            if (start != source.size()-1)
                sexps.push_back(source.substr(start, source.size()-1-start));
            likely_assert(sexps.front().sexps.empty(), "unexpected operand: %s", source.c_str());
            op = sexps.front().op;
            sexps.erase(sexps.begin());
        } else {
            likely_assert(source.find(' ') == string::npos, "unexpected ' ' in: %s", source.c_str());
            op = source;
        }
    }
};

struct KernelInfo
{
    vector<TypedValue> srcs;
    TypedValue i, c, x, y, t;
    likely_type dims;
    KernelInfo(const KernelBuilder &kernel, const vector<TypedValue> &srcs_, const TypedValue &dst, Value *i_)
        : srcs(srcs_), i(i_, likely_type_i32)
    {
        c.type = x.type = y.type = t.type = i.type;
        kernel.deindex(dst, i, &c.value, &x.value, &y.value, &t.value);
        dims = likely_type_null;
        for (const TypedValue &src : srcs)
            dims |= (src.type & likely_type_multi_dimension);
    }
};

struct Operation
{
    static map<string, const Operation*> operations;
    static void add(const Operation *operation)
    {
        assert(operations.find(operation->name()) == operations.end());
        operations.insert(pair<string, const Operation*>(operation->name(), operation));
    }

    virtual ~Operation() {}
    virtual string name() const = 0;
    virtual TypedValue call(KernelBuilder &kernel, const KernelInfo &info, const vector<TypedValue> &args) const = 0;
};
map<string, const Operation*> Operation::operations;

template <class T>
struct RegisterOperation
{
    RegisterOperation()
    {
        Operation::add(new T());
    }
};
#define LIKELY_REGISTER(OPERATION) static struct RegisterOperation<OPERATION> Register##OPERATION;

class NullaryOperation : public Operation
{
    TypedValue call(KernelBuilder &kernel, const KernelInfo &info, const vector<TypedValue> &args) const
    {
        assert(args.empty());
        return callNullary(kernel, info);
    }
    virtual TypedValue callNullary(KernelBuilder &kernel, const KernelInfo &info) const = 0;
};

class IndexOperation : public NullaryOperation
{
    TypedValue callNullary(KernelBuilder &kernel, const KernelInfo &info) const
    {
        (void) kernel;
        return callIndex(info);
    }
    virtual TypedValue callIndex(const KernelInfo &info) const = 0;
};

class iOperation : public IndexOperation
{
    string name() const { return "i"; }
    TypedValue callIndex(const KernelInfo &info) const
    {
        return info.i;
    }
};
LIKELY_REGISTER(iOperation)

class cOperation : public IndexOperation
{
    string name() const { return "c"; }
    TypedValue callIndex(const KernelInfo &info) const
    {
        return info.c;
    }
};
LIKELY_REGISTER(cOperation)

class xOperation : public IndexOperation
{
    string name() const { return "x"; }
    TypedValue callIndex(const KernelInfo &info) const
    {
        return info.x;
    }
};
LIKELY_REGISTER(xOperation)

class yOperation : public IndexOperation
{
    string name() const { return "y"; }
    TypedValue callIndex(const KernelInfo &info) const
    {
        return info.y;
    }
};
LIKELY_REGISTER(yOperation)

class tOperation : public IndexOperation
{
    string name() const { return "t"; }
    TypedValue callIndex(const KernelInfo &info) const
    {
        return info.t;
    }
};
LIKELY_REGISTER(tOperation)

class UnaryOperation : public Operation
{
    TypedValue call(KernelBuilder &kernel, const KernelInfo &info, const vector<TypedValue> &args) const
    {
        assert(args.size() == 1);
        return callUnary(kernel, info, args[0]);
    }
    virtual TypedValue callUnary(KernelBuilder &kernel, const KernelInfo &info, TypedValue arg) const = 0;
};

class UnaryMathOperation : public UnaryOperation
{
    TypedValue callUnary(KernelBuilder &kernel, const KernelInfo &info, TypedValue x) const
    {
        (void) info;
        x = kernel.cast(x, KernelBuilder::validFloatType(x.type));
        vector<Type*> args;
        args.push_back(x.value->getType());
        return TypedValue(kernel.b->CreateCall(Intrinsic::getDeclaration(kernel.m, id(), args), x), x.type);
    }
    virtual Intrinsic::ID id() const = 0;
};

#define LIKELY_REGISTER_UNARY_MATH(OPERATION)                 \
class OPERATION##Operation : public UnaryMathOperation        \
{                                                             \
    string name() const { return #OPERATION; }                \
    Intrinsic::ID id() const { return Intrinsic::OPERATION; } \
};                                                            \
LIKELY_REGISTER(OPERATION##Operation)                         \

LIKELY_REGISTER_UNARY_MATH(sqrt)
LIKELY_REGISTER_UNARY_MATH(sin)
LIKELY_REGISTER_UNARY_MATH(cos)
LIKELY_REGISTER_UNARY_MATH(exp)
LIKELY_REGISTER_UNARY_MATH(exp2)
LIKELY_REGISTER_UNARY_MATH(log)
LIKELY_REGISTER_UNARY_MATH(log10)
LIKELY_REGISTER_UNARY_MATH(log2)
LIKELY_REGISTER_UNARY_MATH(fabs)
LIKELY_REGISTER_UNARY_MATH(floor)
LIKELY_REGISTER_UNARY_MATH(ceil)
LIKELY_REGISTER_UNARY_MATH(trunc)
LIKELY_REGISTER_UNARY_MATH(rint)
LIKELY_REGISTER_UNARY_MATH(nearbyint)
LIKELY_REGISTER_UNARY_MATH(round)

class BinaryOperation : public Operation
{
    TypedValue call(KernelBuilder &kernel, const KernelInfo &info, const vector<TypedValue> &args) const
    {
        assert(args.size() == 2);
        return callBinary(kernel, info, args[0], args[1]);
    }
    virtual TypedValue callBinary(KernelBuilder &kernel, const KernelInfo &info, TypedValue arg1, TypedValue arg2) const = 0;
};

class castOperation : public BinaryOperation
{
    string name() const { return "cast"; }
    TypedValue callBinary(KernelBuilder &kernel, const KernelInfo &info, TypedValue x, TypedValue type) const
    {
        (void) info;
        return kernel.cast(x, LLVM_VALUE_TO_INT(type.value));
    }
};
LIKELY_REGISTER(castOperation)

class ArithmeticOperation : public BinaryOperation
{
    TypedValue callBinary(KernelBuilder &kernel, const KernelInfo &info, TypedValue arg1, TypedValue arg2) const
    {
        (void) info;
        likely_type type = likely_type_from_types(arg1, arg2);
        return callArithmetic(kernel.b, kernel.cast(arg1, type), kernel.cast(arg2, type), type);
    }
    virtual TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const = 0;
};

class addOperation : public ArithmeticOperation
{
    string name() const { return "+"; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(b->CreateFAdd(lhs, rhs), type);
        } else {
            if (likely_saturation(type)) {
                if (likely_signed(type)) {
                    const int depth = likely_depth(type);
                    Value *result = b->CreateAdd(lhs, rhs);
                    Value *overflowResult = b->CreateAdd(b->CreateLShr(lhs, depth-1), KernelBuilder::intMax(depth));
                    Value *overflowCondition = b->CreateICmpSGE(b->CreateOr(b->CreateXor(lhs.value, rhs.value), b->CreateNot(b->CreateXor(rhs, result))), KernelBuilder::zero(depth));
                    return TypedValue(b->CreateSelect(overflowCondition, overflowResult, result), type);
                } else {
                    Value *result = b->CreateAdd(lhs, rhs);
                    Value *overflow = b->CreateNeg(b->CreateZExt(b->CreateICmpULT(result, lhs),
                                                   Type::getIntNTy(getGlobalContext(), likely_depth(type))));
                    return TypedValue(b->CreateOr(result, overflow), type);
                }
            } else {
                return TypedValue(b->CreateAdd(lhs, rhs), type);
            }
        }
    }
};
LIKELY_REGISTER(addOperation)

class subtractOperation : public ArithmeticOperation
{
    string name() const { return "-"; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(b->CreateFSub(lhs, rhs), type);
        } else {
            if (likely_saturation(type)) {
                if (likely_signed(type)) {
                    const int depth = likely_depth(type);
                    Value *result = b->CreateSub(lhs, rhs);
                    Value *overflowResult = b->CreateAdd(b->CreateLShr(lhs, depth-1), KernelBuilder::intMax(depth));
                    Value *overflowCondition = b->CreateICmpSLT(b->CreateAnd(b->CreateXor(lhs.value, rhs.value), b->CreateXor(lhs, result)), KernelBuilder::zero(depth));
                    return TypedValue(b->CreateSelect(overflowCondition, overflowResult, result), type);
                } else {
                    Value *result = b->CreateSub(lhs, rhs);
                    Value *overflow = b->CreateNeg(b->CreateZExt(b->CreateICmpULE(result, lhs),
                                                   Type::getIntNTy(getGlobalContext(), likely_depth(type))));
                    return TypedValue(b->CreateAnd(result, overflow), type);
                }
            } else {
                return TypedValue(b->CreateSub(lhs, rhs), type);
            }
        }
    }
};
LIKELY_REGISTER(subtractOperation)

class multiplyOperation : public ArithmeticOperation
{
    string name() const { return "*"; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(b->CreateFMul(lhs, rhs), type);
        } else {
            if (likely_saturation(type)) {
                const int depth = likely_depth(type);
                Type *originalType = Type::getIntNTy(getGlobalContext(), depth);
                Type *extendedType = Type::getIntNTy(getGlobalContext(), 2*depth);
                Value *result = b->CreateMul(b->CreateZExt(lhs, extendedType),
                                             b->CreateZExt(rhs, extendedType));
                Value *lo = b->CreateTrunc(result, originalType);

                if (likely_signed(type)) {
                    Value *hi = b->CreateTrunc(b->CreateAShr(result, depth), originalType);
                    Value *overflowResult = b->CreateAdd(b->CreateLShr(b->CreateXor(lhs.value, rhs.value), depth-1), KernelBuilder::intMax(depth));
                    Value *overflowCondition = b->CreateICmpNE(hi, b->CreateAShr(lo, depth-1));
                    return TypedValue(b->CreateSelect(overflowCondition, overflowResult, b->CreateTrunc(result, lhs.value->getType())), type);
                } else {
                    Value *hi = b->CreateTrunc(b->CreateLShr(result, depth), originalType);
                    Value *overflow = b->CreateNeg(b->CreateZExt(b->CreateICmpNE(hi, KernelBuilder::zero(depth)), originalType));
                    return TypedValue(b->CreateOr(lo, overflow), type);
                }
            } else {
                return TypedValue(b->CreateMul(lhs, rhs), type);
            }
        }
    }
};
LIKELY_REGISTER(multiplyOperation)

class divideOperation : public ArithmeticOperation
{
    string name() const { return "/"; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue n, TypedValue d, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(b->CreateFDiv(n, d), type);
        } else {
            if (likely_signed(type)) {
                if (likely_saturation(type)) {
                    const int depth = likely_depth(type);
                    Value *safe_i = b->CreateAdd(n, b->CreateZExt(b->CreateICmpNE(b->CreateOr(b->CreateAdd(d, KernelBuilder::constant(1, depth)), b->CreateAdd(n, KernelBuilder::intMin(depth))), KernelBuilder::zero(depth)), n.value->getType()));
                    return TypedValue(b->CreateSDiv(safe_i, d), type);
                } else {
                    return TypedValue(b->CreateSDiv(n, d), type);
                }
            } else {
                return TypedValue(b->CreateUDiv(n, d), type);
            }
        }
    }
};
LIKELY_REGISTER(divideOperation)

class ltOperation : public ArithmeticOperation
{
    string name() const { return "<"; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        Value *comp = likely_floating(type) ? b->CreateFCmpOLT(lhs, rhs) : (likely_signed(type) ? b->CreateICmpSLT(lhs, rhs) : b->CreateICmpULT(lhs, rhs));
        return TypedValue(comp, type);
    }
};
LIKELY_REGISTER(ltOperation)

class leOperation : public ArithmeticOperation
{
    string name() const { return "<="; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        Value *comp = likely_floating(type) ? b->CreateFCmpOLE(lhs, rhs) : (likely_signed(type) ? b->CreateICmpSLE(lhs, rhs) : b->CreateICmpULE(lhs, rhs));
        return TypedValue(comp, type);
    }
};
LIKELY_REGISTER(leOperation)

class gtOperation : public ArithmeticOperation
{
    string name() const { return ">"; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        Value *comp = likely_floating(type) ? b->CreateFCmpOGT(lhs, rhs) : (likely_signed(type) ? b->CreateICmpSGT(lhs, rhs) : b->CreateICmpUGT(lhs, rhs));
        return TypedValue(comp, type);
    }
};
LIKELY_REGISTER(gtOperation)

class geOperation : public ArithmeticOperation
{
    string name() const { return ">="; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        Value *comp = likely_floating(type) ? b->CreateFCmpOGE(lhs, rhs) : (likely_signed(type) ? b->CreateICmpSGE(lhs, rhs) : b->CreateICmpUGE(lhs, rhs));
        return TypedValue(comp, type);
    }
};
LIKELY_REGISTER(geOperation)

class eqOperation : public ArithmeticOperation
{
    string name() const { return "=="; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        Value *comp = likely_floating(type) ? b->CreateFCmpOEQ(lhs, rhs) : b->CreateICmpEQ(lhs, rhs);
        return TypedValue(comp, type);
    }
};
LIKELY_REGISTER(eqOperation)

class neOperation : public ArithmeticOperation
{
    string name() const { return "!="; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        Value *comp = likely_floating(type) ? b->CreateFCmpONE(lhs, rhs) : b->CreateICmpNE(lhs, rhs);
        return TypedValue(comp, type);
    }
};
LIKELY_REGISTER(neOperation)

class BinaryMathOperation : public BinaryOperation
{
    TypedValue callBinary(KernelBuilder &kernel, const KernelInfo &info, TypedValue x, TypedValue n) const
    {
        (void) info;
        const likely_type type = nIsInteger() ? x.type : likely_type_from_types(x, n);
        x = kernel.cast(x, KernelBuilder::validFloatType(type));
        n = kernel.cast(n, nIsInteger() ? likely_type_i32 : x.type);
        vector<Type*> args;
        args.push_back(x.value->getType());
        return TypedValue(kernel.b->CreateCall2(Intrinsic::getDeclaration(kernel.m, id(), args), x, n), x.type);
    }
    virtual Intrinsic::ID id() const = 0;
    virtual bool nIsInteger() const { return false; }
};

class powiOperation : public BinaryMathOperation
{
    string name() const { return "powi"; }
    Intrinsic::ID id() const { return Intrinsic::powi; }
    bool nIsInteger() const { return true; }
};
LIKELY_REGISTER(powiOperation)

class powOperation : public BinaryMathOperation
{
    string name() const { return "pow"; }
    Intrinsic::ID id() const { return Intrinsic::pow; }
};
LIKELY_REGISTER(powOperation)

class copysignOperation : public BinaryMathOperation
{
    string name() const { return "copysign"; }
    Intrinsic::ID id() const { return Intrinsic::copysign; }
};
LIKELY_REGISTER(copysignOperation)

class TernaryOperation : public Operation
{
    TypedValue call(KernelBuilder &kernel, const KernelInfo &info, const vector<TypedValue> &args) const
    {
        assert(args.size() == 3);
        return callTernary(kernel, info, args[0], args[1], args[2]);
    }
    virtual TypedValue callTernary(KernelBuilder &kernel, const KernelInfo &info, TypedValue arg1, TypedValue arg2, TypedValue arg3) const = 0;
};

class fmaOperation : public TernaryOperation
{
    string name() const { return "fma"; }
    TypedValue callTernary(KernelBuilder &kernel, const KernelInfo &info, TypedValue a, TypedValue x, TypedValue c) const
    {
        (void) info;
        const likely_type type = likely_type_from_types(likely_type_from_types(a, x), c);
        x = kernel.cast(x, KernelBuilder::validFloatType(type));
        a = kernel.cast(a, x.type);
        c = kernel.cast(c, x.type);
        vector<Type*> args;
        args.push_back(x.value->getType());
        return TypedValue(kernel.b->CreateCall3(Intrinsic::getDeclaration(kernel.m, Intrinsic::fma, args), x, a, c), x.type);
    }
};
LIKELY_REGISTER(fmaOperation)

struct LikelyKernelOptimizationPass : public FunctionPass
{
    static char ID;
    LikelyKernelOptimizationPass() : FunctionPass(ID) {}
    virtual bool runOnFunction(Function &F) {
      DEBUG(dbgs() << "LKO: " << F.getName() << "\n");
      return false;
    }
};
char LikelyKernelOptimizationPass::ID = 0;
static RegisterPass<LikelyKernelOptimizationPass> RegisterLikelyKernelOptimizationPass("likely", "Likely Kernel Optimization Pass", false, false);

class FunctionBuilder
{
    string name;
    SExp sexp;
    vector<likely_type> types;
    Module *module;
    ExecutionEngine *executionEngine;
    TargetMachine *targetMachine;

public:
    FunctionBuilder(const string &name_, const SExp &sexp_, const vector<likely_type> &types_)
        : name(name_), sexp(sexp_), types(types_)
    {
        module = new Module(name, getGlobalContext());
        likely_assert(module != NULL, "failed to create LLVM Module");
        module->setTargetTriple(sys::getProcessTriple());

        string error;
        EngineBuilder engineBuilder(module);
        engineBuilder.setMCPU(sys::getHostCPUName());
        engineBuilder.setEngineKind(EngineKind::JIT);
        engineBuilder.setOptLevel(CodeGenOpt::Aggressive);
        engineBuilder.setErrorStr(&error);
        engineBuilder.setUseMCJIT(true);

        executionEngine = engineBuilder.create();
        likely_assert(executionEngine != NULL, "failed to create LLVM ExecutionEngine with error: %s", error.c_str());

        targetMachine = engineBuilder.selectTarget();
        likely_assert(targetMachine != NULL, "failed to create LLVM TargetMachine with error: %s", error.c_str());
    }

    ~FunctionBuilder()
    {
        delete targetMachine;
        delete executionEngine;
        delete module;
    }

    void *getPointerToFunction()
    {
        Function *function = module->getFunction(name);
        if (function != NULL)
            return executionEngine->getPointerToFunction(function);
        function = getFunction(name, module, types.size(), PointerType::getUnqual(TheMatrixStruct));

        Function *thunk;
        likely_type dstType;
        {
            thunk = getFunction(name+"_thunk", module, types.size(), Type::getVoidTy(getGlobalContext()), PointerType::getUnqual(TheMatrixStruct), Type::getInt32Ty(getGlobalContext()), Type::getInt32Ty(getGlobalContext()));
            vector<TypedValue> srcs;
            getValues(thunk, types, srcs);
            TypedValue stop = srcs.back(); srcs.pop_back();
            stop.value->setName("stop");
            stop.type = likely_type_i32;
            TypedValue start = srcs.back(); srcs.pop_back();
            start.value->setName("start");
            start.type = likely_type_i32;
            TypedValue dst = srcs.back(); srcs.pop_back();
            dst.value->setName("dst");

            BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", thunk);
            IRBuilder<> builder(entry);
            KernelBuilder kernel(module, &builder, thunk);
            Value *i = kernel.beginLoop(entry, start, stop).i;
            KernelInfo info(kernel, srcs, dst, i);

            TypedValue result = generateKernelRecursive(kernel, info, sexp);

            dstType = result.type;
            kernel.store(TypedValue(dst.value, dstType), i, result);
            kernel.endLoop();
            builder.CreateRetVoid();

            FunctionPassManager functionPassManager(module);
            functionPassManager.add(createVerifierPass(PrintMessageAction));
            functionPassManager.add(new LikelyKernelOptimizationPass());
            targetMachine->addAnalysisPasses(functionPassManager);
            functionPassManager.add(new TargetLibraryInfo(Triple(module->getTargetTriple())));
            functionPassManager.add(new DataLayout(module));
            functionPassManager.add(createBasicAliasAnalysisPass());
            functionPassManager.add(createLICMPass());
            functionPassManager.add(createLoopVectorizePass());
            functionPassManager.add(createInstructionCombiningPass());
            functionPassManager.add(createEarlyCSEPass());
            functionPassManager.add(createCFGSimplificationPass());
            functionPassManager.doInitialization();
//            DebugFlag = true;
            functionPassManager.run(*thunk);
        }

        static FunctionType* LikelyNewSignature = NULL;
        if (LikelyNewSignature == NULL) {
            Type *newReturn = PointerType::getUnqual(TheMatrixStruct);
            vector<Type*> newParameters;
            newParameters.push_back(Type::getInt32Ty(getGlobalContext())); // type
            newParameters.push_back(Type::getInt32Ty(getGlobalContext())); // channels
            newParameters.push_back(Type::getInt32Ty(getGlobalContext())); // columns
            newParameters.push_back(Type::getInt32Ty(getGlobalContext())); // rows
            newParameters.push_back(Type::getInt32Ty(getGlobalContext())); // frames
            newParameters.push_back(Type::getInt8PtrTy(getGlobalContext())); // data
            newParameters.push_back(Type::getInt8Ty(getGlobalContext())); // copy
            LikelyNewSignature = FunctionType::get(newReturn, newParameters, false);
        }
        Function *likelyNew = Function::Create(LikelyNewSignature, GlobalValue::ExternalLinkage, "likely_new", module);
        likelyNew->setCallingConv(CallingConv::C);
        likelyNew->setDoesNotAlias(0);
        likelyNew->setDoesNotAlias(6);
        likelyNew->setDoesNotCapture(6);

        vector<TypedValue> srcs;
        getValues(function, types, srcs);
        BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", function);
        IRBuilder<> builder(entry);
        KernelBuilder kernel(module, &builder, function);

        Value *dstChannels = NULL, *dstColumns = NULL, *dstRows = NULL, *dstFrames = NULL;
        for (size_t i=0; i<types.size(); i++) {
            if (!dstChannels && likely_multi_channel(types[i])) dstChannels = kernel.channels(srcs[i]);
            if (!dstColumns  && likely_multi_column(types[i]))  dstColumns  = kernel.columns(srcs[i]);
            if (!dstRows     && likely_multi_row(types[i]))     dstRows     = kernel.rows(srcs[i]);
            if (!dstFrames   && likely_multi_frame(types[i]))   dstFrames   = kernel.frames(srcs[i]);
        }
        if (!dstChannels) { dstChannels = kernel.constant(1); assert(!likely_multi_channel(dstType)); }
        if (!dstColumns)  { dstColumns  = kernel.constant(1); assert(!likely_multi_column(dstType));  }
        if (!dstRows)     { dstRows     = kernel.constant(1); assert(!likely_multi_row(dstType));     }
        if (!dstFrames)   { dstFrames   = kernel.constant(1); assert(!likely_multi_frame(dstType));   }

        std::vector<Value*> likelyNewArgs;
        likelyNewArgs.push_back(kernel.type(dstType));
        likelyNewArgs.push_back(dstChannels);
        likelyNewArgs.push_back(dstColumns);
        likelyNewArgs.push_back(dstRows);
        likelyNewArgs.push_back(dstFrames);
        likelyNewArgs.push_back(ConstantPointerNull::get(Type::getInt8PtrTy(getGlobalContext())));
        likelyNewArgs.push_back(kernel.constant(0, 8));
        Value *dst = builder.CreateCall(likelyNew, likelyNewArgs);

        Value *kernelSize = builder.CreateMul(builder.CreateMul(builder.CreateMul(dstChannels, dstColumns), dstRows), dstFrames);
        if (likely_parallel(types[0])) {
            static FunctionType *likelyForkType = NULL;
            if (likelyForkType == NULL) {
                vector<Type*> likelyForkParameters;
                likelyForkParameters.push_back(thunk->getType());
                likelyForkParameters.push_back(Type::getInt8Ty(getGlobalContext()));
                likelyForkParameters.push_back(Type::getInt32Ty(getGlobalContext()));
                likelyForkParameters.push_back(PointerType::getUnqual(TheMatrixStruct));
                Type *likelyForkReturn = Type::getVoidTy(getGlobalContext());
                likelyForkType = FunctionType::get(likelyForkReturn, likelyForkParameters, true);
            }
            Function *likelyFork = Function::Create(likelyForkType, GlobalValue::ExternalLinkage, "likely_fork", module);
            likelyFork->setCallingConv(CallingConv::C);
            likelyFork->setDoesNotCapture(4);
            likelyFork->setDoesNotAlias(4);

            vector<Value*> likelyForkArgs;
            likelyForkArgs.push_back(module->getFunction(thunk->getName()));
            likelyForkArgs.push_back(KernelBuilder::constant(types.size(), 8));
            likelyForkArgs.push_back(kernelSize);
            likelyForkArgs.insert(likelyForkArgs.end(), srcs.begin(), srcs.end());
            likelyForkArgs.push_back(dst);
            builder.CreateCall(likelyFork, likelyForkArgs);
        } else {
            vector<Value*> thunkArgs;
            for (const TypedValue &src : srcs)
                thunkArgs.push_back(src.value);
            thunkArgs.push_back(dst);
            thunkArgs.push_back(KernelBuilder::zero());
            thunkArgs.push_back(kernelSize);
            builder.CreateCall(thunk, thunkArgs);
        }

        builder.CreateRet(dst);

        FunctionPassManager functionPassManager(module);
        functionPassManager.add(createVerifierPass(PrintMessageAction));
        functionPassManager.add(createInstructionCombiningPass());
        functionPassManager.run(*function);

//        module->dump();
        executionEngine->finalizeObject();
        return executionEngine->getPointerToFunction(function);
    }

private:
    static Function *getFunction(const string &name, Module *m, likely_arity arity, Type *ret, Type *dst = NULL, Type *start = NULL, Type *stop = NULL)
    {
        PointerType *matrixPointer = PointerType::getUnqual(TheMatrixStruct);
        Function *function;
        switch (arity) {
          case 0: function = cast<Function>(m->getOrInsertFunction(name, ret, dst, start, stop, NULL)); break;
          case 1: function = cast<Function>(m->getOrInsertFunction(name, ret, matrixPointer, dst, start, stop, NULL)); break;
          case 2: function = cast<Function>(m->getOrInsertFunction(name, ret, matrixPointer, matrixPointer, dst, start, stop, NULL)); break;
          case 3: function = cast<Function>(m->getOrInsertFunction(name, ret, matrixPointer, matrixPointer, matrixPointer, dst, start, stop, NULL)); break;
          default: { function = NULL; likely_assert(false, "FunctionBuilder::getFunction invalid arity: %d", arity); }
        }
        function->addFnAttr(Attribute::NoUnwind);
        function->setCallingConv(CallingConv::C);
        if (ret->isPointerTy())
            function->setDoesNotAlias(0);
        size_t num_mats = arity;
        if (dst) num_mats++;
        for (size_t i=0; i<num_mats; i++) {
            function->setDoesNotAlias(i+1);
            function->setDoesNotCapture(i+1);
        }
        return function;
    }

    static void getValues(Function *function, const vector<likely_type> &types, vector<TypedValue> &srcs)
    {
        Function::arg_iterator args = function->arg_begin();
        likely_arity arity = 0;
        while (args != function->arg_end()) {
            Value *src = args++;
            stringstream name; name << "__" << int(arity);
            src->setName(name.str());
            srcs.push_back(TypedValue(src, arity < types.size() ? types[arity] : likely_type_null));
            arity++;
        }
    }

    TypedValue generateKernelRecursive(KernelBuilder &kernel, const KernelInfo &info, const SExp &expression)
    {
        vector<TypedValue> operands;
        for (const SExp &operand : expression.sexps)
            operands.push_back(generateKernelRecursive(kernel, info, operand));
        const string &op = expression.op;

        map<string,const Operation*>::iterator it = Operation::operations.find(op);
        if (it != Operation::operations.end())
            return it->second->call(kernel, info, operands);

        static regex constant("(-?\\d*\\.?\\d+)([uif]\\d*)?");
        std::smatch sm;
        if (regex_match(op, sm, constant)) {
            assert(sm.size() == 3);
            const double value = atof(string(sm[1]).c_str());
            const likely_type type = string(sm[2]).empty()
                                   ? likely_type_from_value(value)
                                   : likely_type_from_string(string(sm[2]).c_str());
            return KernelBuilder::constant(value, type);
        } else if (op.substr(0,2) == "__") {
            int index = atoi(op.substr(2, op.size()-2).c_str());
            const TypedValue matrix = info.srcs[index];
            Value *matrix_i;
            if ((matrix.type & likely_type_multi_dimension) == info.dims) {
                // This matrix has the same dimensionality as the output
                matrix_i = info.i;
            } else {
                Value *c, *x, *y, *t;
                kernel.deindex(matrix, info.i, &c, &x, &y, &t);
                matrix_i = kernel.index(matrix, c, x, y, t);
            }
            return TypedValue(kernel.load(matrix, matrix_i), matrix.type);
        }

        likely_assert(false, "unrecognized literal: %s", op.c_str());
        return TypedValue();
    }
};

// Control parallel execution
static vector<mutex*> workers;
static mutex workersActive;
static atomic_uint workersRemaining(0);
static void *currentThunk = NULL;
static likely_arity thunkArity = 0;
static likely_size thunkSize = 0;
static likely_const_mat thunkMatricies[LIKELY_NUM_ARITIES+1];

// Final three parameters are: dst, start, stop
typedef void (*likely_kernel_0)(likely_mat, likely_size, likely_size);
typedef void (*likely_kernel_1)(likely_const_mat, likely_mat, likely_size, likely_size);
typedef void (*likely_kernel_2)(likely_const_mat, likely_const_mat, likely_mat, likely_size, likely_size);
typedef void (*likely_kernel_3)(likely_const_mat, likely_const_mat, likely_const_mat, likely_mat, likely_size, likely_size);

static void executeWorker(int id, int numWorkers)
{
    // There are hardware_concurrency-1 helper threads and the main thread with id = 0
    const likely_size step = (thunkSize+numWorkers-1)/numWorkers;
    const likely_size start = id * step;
    const likely_size stop = std::min((id+1)*step, thunkSize);
    if (start >= stop) return;

    switch (thunkArity) {
      case 0: reinterpret_cast<likely_kernel_0>(currentThunk)((likely_mat)thunkMatricies[0], start, stop); break;
      case 1: reinterpret_cast<likely_kernel_1>(currentThunk)(thunkMatricies[0], (likely_mat)thunkMatricies[1], start, stop); break;
      case 2: reinterpret_cast<likely_kernel_2>(currentThunk)(thunkMatricies[0], thunkMatricies[1], (likely_mat)thunkMatricies[2], start, stop); break;
      case 3: reinterpret_cast<likely_kernel_3>(currentThunk)(thunkMatricies[0], thunkMatricies[1], thunkMatricies[2], (likely_mat)thunkMatricies[3], start, stop); break;
      default: likely_assert(false, "executeWorker invalid arity: %d", thunkArity);
    }
}

static void workerThread(int id, int numWorkers)
{
    while (true) {
        workers[id]->lock();
        executeWorker(id, numWorkers);
        if (--workersRemaining == 0)
            workersActive.unlock();
    }
}

void *likely_compile(likely_description description, likely_arity n, likely_type type, ...)
{
    vector<likely_type> srcs;
    va_list ap;
    va_start(ap, type);
    for (int i=0; i<n; i++) {
        srcs.push_back(type);
        type = va_arg(ap, likely_type);
    }
    va_end(ap);
    return likely_compile_n(description, n, srcs.data());
}

void *likely_compile_n(likely_description description, likely_arity n, likely_type *types_)
{
    const vector<likely_type> types(types_,types_+n);
    static map<string,FunctionBuilder*> kernels;
    static recursive_mutex makerLock;
    lock_guard<recursive_mutex> lock(makerLock);

    if (TheMatrixStruct == NULL) {
        // Initialize Likely
        InitializeNativeTarget();
        InitializeNativeTargetAsmPrinter();
        InitializeNativeTargetAsmParser();
        initializeScalarOpts(*PassRegistry::getPassRegistry());

        TheMatrixStruct = StructType::create("likely_matrix",
                                             Type::getInt8PtrTy(getGlobalContext()), // data
                                             Type::getInt32Ty(getGlobalContext()),   // type
                                             Type::getInt32Ty(getGlobalContext()),   // channels
                                             Type::getInt32Ty(getGlobalContext()),   // columns
                                             Type::getInt32Ty(getGlobalContext()),   // rows
                                             Type::getInt32Ty(getGlobalContext()),   // frames
                                             PointerType::getUnqual(StructType::create(getGlobalContext(), "likely_matrix_private")), // d_ptr
                                             NULL);

        const int numWorkers = std::max((int)thread::hardware_concurrency(), 1);
        workers.push_back(NULL); // main thread = 0
        for (int i=1; i<numWorkers; i++) {
            mutex *m = new mutex();
            m->lock();
            workers.push_back(m);
            thread(workerThread, i, numWorkers).detach();
        }
    }

    string source = description;
    if (source.compare(0, 1, "(")) {
        // It needs to be interpreted (usually the case)
        static lua_State *L = NULL;
        stringstream command; command << "return tostring(" << description << ")";
        L = likely_exec(command.str().c_str(), L);
        source = lua_tostring(L, -1);
    }

    static int index = 0;
    stringstream name; name << "likely_" << index++;

    stringstream uniqueIDStream;
    for (likely_type type : types)
        uniqueIDStream << likely_type_to_string(type) << "_";
    uniqueIDStream << "_" << source;
    const string uniqueID = uniqueIDStream.str();

    map<string,FunctionBuilder*>::const_iterator it = kernels.find(uniqueID);
    if (it == kernels.end()) {
        kernels.insert(pair<string,FunctionBuilder*>(uniqueID, new FunctionBuilder(name.str(), SExp(source), types)));
        it = kernels.find(uniqueID);
    }
    return it->second->getPointerToFunction();
}

static int lua_likely_compile(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args >= 1, "'compile' expected at least one argument");

    vector<likely_type> types;
    for (int i=2; i<=args; i++)
        types.push_back(checkLuaMat(L, i)->type);

    lua_getglobal(L, "tostring");
    lua_pushvalue(L, 1);
    lua_call(L, 1, 1);

    // Compile the function
    lua_pushlightuserdata(L, likely_compile_n(lua_tostring(L, -1), types.size(), types.data()));
    return 1;
}

static int lua_likely_closure(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, (args >= 3) && (args <= 4), "'closure' expected 3-4 arguments, got: %d", args);

    lua_newtable(L);
    lua_pushstring(L, "source");
    lua_pushvalue(L, 1);
    lua_settable(L, -3);
    lua_pushstring(L, "documentation");
    lua_pushvalue(L, 2);
    lua_settable(L, -3);
    lua_pushstring(L, "parameters");
    lua_pushvalue(L, 3);
    lua_settable(L, -3);
    if (args >= 4) {
        lua_pushstring(L, "binary");
        lua_pushvalue(L, 4);
        lua_settable(L, -3);
    }

    lua_newtable(L);
    lua_pushnil(L);
    int i = 1;
    while (lua_next(L, 3)) {
        // All parameters can be set by name
        lua_pushinteger(L, 1);
        lua_gettable(L, -2);
        lua_pushvalue(L, -3);
        lua_settable(L, -5);

        // Unassigned parameters can also be set by index
        if (lua_objlen(L, -1) < 3) {
            lua_pushinteger(L, i++);
            lua_pushvalue(L, -3);
            lua_settable(L, -5);
        }
        lua_pop(L, 1);
    }
    lua_setfield(L, -2, "parameterLUT");

    luaL_getmetatable(L, "likely_closure");
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_likely__call(lua_State *L)
{
    int args = lua_gettop(L);
    lua_likely_assert(L, args >= 1, "'__call' expected at least one argument");

    // Copy the arguments already in the closure to a new table
    lua_newtable(L);
    lua_getfield(L, 1, "parameters");
    lua_pushnil(L);
    bool overrideArguments = true;
    while (lua_next(L, -2)) {
        overrideArguments = overrideArguments && (lua_objlen(L, -1) >= 3);
        lua_newtable(L);
        lua_pushnil(L);
        while (lua_next(L, -3)) {
            lua_pushvalue(L, -2);
            lua_insert(L, -2);
            lua_settable(L, -4);
        }
        lua_pushvalue(L, -3);
        lua_insert(L, -2);
        lua_settable(L, -6);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    // Using {} syntax?
    const bool curry = (args == 2) && lua_istable(L, 2);

    // Add the new arguments
    lua_getfield(L, 1, "parameterLUT");
    if (curry) {
        lua_pushnil(L);
        while (lua_next(L, 2)) {
            lua_pushvalue(L, -2);
            if (!lua_isnumber(L, -1) || !overrideArguments)
                lua_gettable(L, -4);
            lua_gettable(L, -5);
            lua_insert(L, -2);
            lua_pushnumber(L, 3);
            lua_insert(L, -2);
            lua_settable(L, -3);
            lua_pop(L, 1);
        }
    } else {
        for (int i=1; i<args; i++) {
            lua_pushinteger(L, i);
            if (!overrideArguments)
                lua_gettable(L, -2);
            lua_gettable(L, -3);
            lua_pushnumber(L, 3);
            lua_pushvalue(L, i+1);
            lua_settable(L, -3);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    if (curry) {
        // Return a new closure
        lua_getglobal(L, "closure");
        lua_getfield(L, 1, "source");
        lua_getfield(L, 1, "documentation");
        lua_pushvalue(L, -4); // parameters
        lua_getfield(L, 1, "binary");
        lua_call(L, 4, 1);
        return 1;
    }

    // Ensure all the arguments are provided
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        lua_pushnumber(L, 3);
        lua_gettable(L, -2);
        if (lua_isnil(L, -1))
            luaL_error(L, "Insufficient arguments!");
        lua_pop(L, 2);
    }

    // Call the function
    lua_getfield(L, 1, "binary");
    if (lua_isnil(L, -1) || lua_isuserdata(L, -1)) {
        // Convert numbers to matricies
        for (int i=2; i<=args; i++)
            if (lua_isnumber(L, i)) {
                *newLuaMat(L) = likely_scalar(lua_tonumber(L, i));
                lua_replace(L, i);
            }

        // Compile the function if needed
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_getglobal(L, "likely");
            lua_getfield(L, -1, "compile");
            lua_pushvalue(L, 1);
            for (int i=2; i<=args; i++)
                lua_pushvalue(L, i);
            lua_call(L, args, 1);
        }

        // Prepare the JIT function
        void *function = lua_touserdata(L, -1);
        vector<likely_const_mat> mats;
        for (int i=2; i<=args; i++)
            mats.push_back(checkLuaMat(L, i));

        // Call the function, return the result
        likely_mat dst;
        switch (mats.size()) {
          case 0: dst = reinterpret_cast<likely_function_0>(function)(); break;
          case 1: dst = reinterpret_cast<likely_function_1>(function)(mats[0]); break;
          case 2: dst = reinterpret_cast<likely_function_2>(function)(mats[0], mats[1]); break;
          case 3: dst = reinterpret_cast<likely_function_3>(function)(mats[0], mats[1], mats[2]); break;
          default: dst = NULL; likely_assert(false, "__call invalid arity: %d", mats.size());
        }
        *newLuaMat(L) = dst;
    } else {
        // Core function
        lua_pushnil(L);
        const int argsIndex = lua_gettop(L) - 2;
        while (lua_next(L, argsIndex)) {
            lua_pushinteger(L, 3);
            lua_gettable(L, -2);
            lua_insert(L, -3);
            lua_pop(L, 1);
        }
        lua_call(L, lua_objlen(L, argsIndex), 1);
    }

    return 1;
}

static int lua_likely__concat(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 2, "__concat expected two arguments, got: %d", args);
    lua_getglobal(L, "closure");

    // source
    lua_getglobal(L, "chain");
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_call(L, 2, 1);

    // documentation
    lua_pushstring(L, "..");

    // parameters
    lua_newtable(L);
    lua_pushinteger(L, 1);
    lua_newtable(L);
    lua_pushinteger(L, 1);
    lua_pushstring(L, "x");
    lua_settable(L, -3);
    lua_pushinteger(L, 2);
    lua_pushstring(L, "operand");
    lua_settable(L, -3);
    lua_settable(L, -3);

    // return a new closure
    lua_call(L, 3, 1);
    return 1;
}

static int lua_likely_closure__tostring(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 1, "__tostring expected one argument, got: %d", args);

    // Setup and call the function
    lua_getfield(L, 1, "source");
    lua_getfield(L, 1, "parameters");
    lua_pushnil(L);
    likely_arity arity = 0;
    while (lua_next(L, -2)) {
        lua_pushinteger(L, 3);
        lua_gettable(L, -2);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            stringstream parameter;
            parameter << "__" << int(arity++);
            lua_pushstring(L, parameter.str().c_str());
        }
        lua_insert(L, -4);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_call(L, lua_gettop(L)-2, 1);
    return 1;
}

void _likely_fork(void *thunk, likely_arity arity, likely_size size, likely_const_mat src, ...)
{
    workersActive.lock();

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

    // Skip myself when starting other threads
    workersRemaining = workers.size()-1;
    for (size_t i=1; i<workers.size(); i++)
        workers[i]->unlock();

    executeWorker(0, workers.size());
    while (workersRemaining > 0) {}
}

int luaopen_likely(lua_State *L)
{
    static const struct luaL_Reg likely_globals[] = {
        {"new", lua_likely_new},
        {"scalar", lua_likely_scalar},
        {"read", lua_likely_read},
        {"closure", lua_likely_closure},
        {"compile", lua_likely_compile},
        {NULL, NULL}
    };

    static const struct luaL_Reg likely_members[] = {
        {"__index", lua_likely__index},
        {"__newindex", lua_likely__newindex},
        {"__tostring", lua_likely__tostring},
        {"__gc", lua_likely__gc},
        {"copy", lua_likely_copy},
        {"get", lua_likely_get},
        {"set", lua_likely_set},
        {"elements", lua_likely_elements},
        {"bytes", lua_likely_bytes},
        {"write", lua_likely_write},
        {"decode", lua_likely_decode},
        {"encode", lua_likely_encode},
        {"render", lua_likely_render},
        {NULL, NULL}
    };

    static const struct luaL_Reg likely_closure[] = {
        {"__call", lua_likely__call},
        {"__concat", lua_likely__concat},
        {"__tostring", lua_likely_closure__tostring},
        {NULL, NULL}
    };

    // Register closure metatable
    luaL_newmetatable(L, "likely_closure");
    luaL_setfuncs(L, likely_closure, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    // Idiom for registering library with member functions
    luaL_newmetatable(L, "likely");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, likely_members, 0);
    luaL_newlib(L, likely_globals);

    typedef pair<const char*, int> type_field;
    vector<type_field> typeFields;
    typeFields.push_back(type_field("null", likely_type_null));
    typeFields.push_back(type_field("depth", likely_type_depth));
    typeFields.push_back(type_field("signed", likely_type_signed));
    typeFields.push_back(type_field("floating", likely_type_floating));
    typeFields.push_back(type_field("u8", likely_type_u8));
    typeFields.push_back(type_field("u16", likely_type_u16));
    typeFields.push_back(type_field("u32", likely_type_u32));
    typeFields.push_back(type_field("u64", likely_type_u64));
    typeFields.push_back(type_field("i8", likely_type_i8));
    typeFields.push_back(type_field("i16", likely_type_i16));
    typeFields.push_back(type_field("i32", likely_type_i32));
    typeFields.push_back(type_field("i64", likely_type_i64));
    typeFields.push_back(type_field("f16", likely_type_f16));
    typeFields.push_back(type_field("f32", likely_type_f32));
    typeFields.push_back(type_field("f64", likely_type_f64));
    typeFields.push_back(type_field("parallel", likely_type_parallel));
    typeFields.push_back(type_field("heterogeneous", likely_type_heterogeneous));
    typeFields.push_back(type_field("multi_channel", likely_type_multi_channel));
    typeFields.push_back(type_field("multi_column", likely_type_multi_column));
    typeFields.push_back(type_field("multi_row", likely_type_multi_row));
    typeFields.push_back(type_field("multi_frame", likely_type_multi_frame));
    typeFields.push_back(type_field("saturation", likely_type_saturation));
    typeFields.push_back(type_field("reserved", likely_type_reserved));
    for (type_field typeField : typeFields) {
        lua_pushstring(L, typeField.first);
        lua_pushinteger(L, typeField.second);
        lua_settable(L, -3);
    }

    return 1;
}

static void toStream(lua_State *L, int index, stringstream &stream, int levels = 1)
{
    lua_pushvalue(L, index);
    const int type = lua_type(L, -1);
    if (type == LUA_TSTRING) {
        stream << lua_tostring(L, -1);
    } else if (type == LUA_TBOOLEAN) {
        stream << (lua_toboolean(L, -1) ? "true" : "false");
    } else if (type == LUA_TNUMBER) {
        stream << lua_tonumber(L, -1);
    } else if (type == LUA_TTABLE) {
        if (levels == 0) {
            stream << "table";
        } else {
            stream << "{";
            lua_pushnil(L);
            bool first = true;
            while (lua_next(L, -2)) {
                if (first) first = false;
                else       stream << ", ";
                if (!lua_isnumber(L, -2)) {
                    toStream(L, -2, stream, levels - 1);
                    stream << "=";
                }
                toStream(L, -1, stream, levels - 1);
                lua_pop(L, 1);
            }
            stream << "}";
        }
    } else {
        stream << lua_typename(L, type);
    }
    lua_pop(L, 1);
}

lua_State *likely_exec(const char *source, lua_State *L)
{
    if (L == NULL) {
        L = luaL_newstate();
        luaL_openlibs(L);
        luaL_requiref(L, "likely", luaopen_likely, 1);
        lua_pop(L, 1);
        luaL_dostring(L, likely_standard_library());
    }

    // Clear the previous stack
    lua_settop(L, 0);

    // Create a sandboxed enviornment
    lua_newtable(L); // _ENV
    lua_newtable(L); // metatable
    lua_getglobal(L, "_G");
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    if (luaL_loadstring(L, source)) return L;
    lua_pushvalue(L, -2);
    lua_setupvalue(L, -2, 1);
    lua_pcall(L, 0, LUA_MULTRET, 0);
    return L; // The sandboxed environment results are now on the top of the stack
}

void likely_stack_dump(lua_State *L, int levels)
{
    if (levels == 0)
        return;

    stringstream stream;
    const int top = lua_gettop(L);
    for (int i=1; i<=top; i++) {
        stream << i << ": ";
        toStream(L, i, stream, levels);
        stream << "\n";
    }
    fprintf(stderr, "Lua stack dump:\n%s", stream.str().c_str());
    lua_likely_assert(L, false, "Lua execution error");
}
