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

static StructType *TheMatrixStruct = NULL;
static const int MaxRegisterWidth = 32; // This should be determined at run time

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

static inline int likely_get(likely_type type, likely_type_field mask) { return type & mask; }
static inline void likely_set(likely_type *type, int i, likely_type_field mask) { *type &= ~mask; *type |= i & mask; }
static inline bool likely_get_bool(likely_type type, likely_type_field mask) { return type & mask; }
static inline void likely_set_bool(likely_type *type, bool b, likely_type_field mask) { b ? *type |= mask : *type &= ~mask; }
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
bool likely_single_channel(likely_type type) { return likely_get_bool(type, likely_type_single_channel); }
void likely_set_single_channel(likely_type *type, bool single_channel) { likely_set_bool(type, single_channel, likely_type_single_channel); }
bool likely_single_column(likely_type type) { return likely_get_bool(type, likely_type_single_column); }
void likely_set_single_column(likely_type *type, bool single_column) { likely_set_bool(type, single_column, likely_type_single_column); }
bool likely_single_row(likely_type type) { return likely_get_bool(type, likely_type_single_row); }
void likely_set_single_row(likely_type *type, bool single_row) { likely_set_bool(type, single_row, likely_type_single_row); }
bool likely_single_frame(likely_type type) { return likely_get_bool(type, likely_type_single_frame); }
void likely_set_single_frame(likely_type *type, bool single_frame) { likely_set_bool(type, single_frame, likely_type_single_frame); }
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
    else if (!strcmp(field, "singleChannel")) lua_pushboolean(L, likely_single_channel(m->type));
    else if (!strcmp(field, "singleColumn"))  lua_pushboolean(L, likely_single_column(m->type));
    else if (!strcmp(field, "singleRow"))     lua_pushboolean(L, likely_single_row(m->type));
    else if (!strcmp(field, "singleFrame"))   lua_pushboolean(L, likely_single_frame(m->type));
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
    if(!lua_isnil(L, -1))
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
    else if (!strcmp(field, "singleChannel")) likely_set_single_channel(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "singleColumn"))  likely_set_single_column(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "singleRow"))     likely_set_single_row(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "singleFrame"))   likely_set_single_frame(&m->type, lua_toboolean(L, 3));
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

// Note: these are currently not used in a thread safe manner
static likely_mat recycledBuffer = NULL;

static likely_data *alignedDataPointer(likely_mat m)
{
    return reinterpret_cast<likely_data*>((uintptr_t(m+1) + sizeof(likely_matrix_private) + (MaxRegisterWidth-1)) & ~uintptr_t(MaxRegisterWidth-1));
}

likely_mat likely_new(likely_type type, likely_size channels, likely_size columns, likely_size rows, likely_size frames, likely_data *data, int8_t copy)
{
    likely_mat m;
    size_t dataBytes = ((data && !copy) ? 0 : uint64_t(likely_depth(type)) * channels * columns * rows * frames / 8);
    const size_t headerBytes = sizeof(likely_matrix) + sizeof(likely_matrix_private) + (MaxRegisterWidth - 1);
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

    assert(alignof(likely_matrix_private) <= alignof(likely_matrix));
    m->d_ptr = reinterpret_cast<likely_matrix_private*>(m+1);
    m->d_ptr->ref_count = 1;
    m->d_ptr->data_bytes = dataBytes;

    if (data && !copy) {
        m->data = data;
    } else {
        m->data = alignedDataPointer(m);
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
      case 7: copy    = lua_toboolean(L, 7);
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

const char *likely_type_to_string(likely_type h)
{
    static string typeString; // Provides return value persistence

    stringstream typeStream;
    typeStream << (likely_floating(h) ? "f" : (likely_signed(h) ? "i" : "u"));
    typeStream << likely_depth(h);

    if (likely_parallel(h))       typeStream << "P";
    if (likely_heterogeneous(h))  typeStream << "H";
    if (likely_single_channel(h)) typeStream << "C";
    if (likely_single_column(h))  typeStream << "X";
    if (likely_single_row(h))     typeStream << "Y";
    if (likely_single_frame(h))   typeStream << "T";
    if (likely_saturation(h))     typeStream << "S";

    typeString = typeStream.str();
    return typeString.c_str();
}

likely_type likely_string_to_type(const char *str)
{
    likely_type t = likely_type_null;
    const size_t len = strlen(str);
    if ((str == NULL) || (len == 0))
        return t;

    if (str[0] == 'f') likely_set_floating(&t, true);
    if (str[0] != 'u') likely_set_signed(&t, true);
    likely_set_depth(&t, atoi(str+1));

    size_t startIndex = 1;
    while ((str[startIndex] >= '0') && (str[startIndex] <= '9'))
        startIndex++;

    for (size_t i=startIndex; i<len; i++) {
        if (str[i] == 'P') likely_set_parallel(&t, true);
        if (str[i] == 'H') likely_set_heterogeneous(&t, true);
        if (str[i] == 'C') likely_set_single_channel(&t, true);
        if (str[i] == 'X') likely_set_single_column(&t, true);
        if (str[i] == 'Y') likely_set_single_row(&t, true);
        if (str[i] == 'T') likely_set_single_frame(&t, true);
        if (str[i] == 'S') likely_set_saturation(&t, true);
    }

    return t;
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

namespace likely
{

struct MatrixBuilder
{
    Module *m;
    IRBuilder<> *b;
    Function *f;
    Twine n;
    likely_type t;
    Value *v;

    struct Loop {
        BasicBlock *body;
        PHINode *i;
        Value *stop;
        MDNode *node;
    };
    stack<Loop> loops;

    MatrixBuilder(Module *module, IRBuilder<> *builder, Function *function, const Twine &name, likely_type type = likely_type_null, Value *value = NULL)
        : m(module), b(builder), f(function), n(name), t(type), v(value) {}
    void reset(IRBuilder<> *builder, Function *function, Value *value) { b = builder; f = function; v = value; }

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
    Constant *autoConstant(double value) const { return likely_floating(t) ? ((likely_depth(t) == 64) ? constant(value) : constant(float(value))) : constant(int(value), likely_depth(t)); }
    AllocaInst *autoAlloca(double value) const { AllocaInst *alloca = b->CreateAlloca(ty(), 0, n); b->CreateStore(autoConstant(value), alloca); return alloca; }

    Value *data(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 0), n+"_data"); }
    Value *data(Value *matrix, Type *type) const { return b->CreatePointerCast(data(matrix), type); }
    Value *type(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 1), n+"_type"); }
    Value *channels(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 2), n+"_channels"); }
    Value *columns(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 3), n+"_columns"); }
    Value *rows(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 4), n+"_rows"); }
    Value *frames(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 5), n+"_frames"); }

    Value *data(bool cast = true) const { return cast ? data(v, ty(true)) : data(v); }
    Value *type() const { return type(v); }
    Value *channels() const { return likely_single_channel(t) ? static_cast<Value*>(one()) : channels(v); }
    Value *columns() const { return likely_single_column(t) ? static_cast<Value*>(one()) : columns(v); }
    Value *rows() const { return likely_single_row(t) ? static_cast<Value*>(one()) : rows(v); }
    Value *frames() const { return likely_single_frame(t) ? static_cast<Value*>(one()) : frames(v); }

    void setData(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 0)); }
    void setType(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 1)); }
    void setChannels(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 2)); }
    void setColumns(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 3)); }
    void setRows(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 4)); }
    void setFrames(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 5)); }

    void setData(Value *value) const { setData(v, value); }
    void setType(Value *value) const { setType(v, value); }
    void setChannels(Value *value) const { setChannels(v, value); }
    void setColumns(Value *value) const { setColumns(v, value); }
    void setRows(Value *value) const { setRows(v, value); }
    void setFrames(Value *value) const { setFrames(v, value); }

    void copyHeaderTo(Value *matrix) const {
        setType(matrix, type());
        setChannels(matrix, channels());
        setColumns(matrix, columns());
        setRows(matrix, rows());
        setFrames(matrix, frames());
    }

    Value *get(int mask) const { return b->CreateAnd(type(), constant(mask, 8*sizeof(likely_type))); }
    void set(int value, int mask) const { setType(b->CreateOr(b->CreateAnd(type(), constant(~mask, 8*sizeof(likely_type))), b->CreateAnd(constant(value, 8*sizeof(likely_type)), constant(mask, 8*sizeof(likely_type))))); }
    void setBit(bool on, int mask) const { on ? setType(b->CreateOr(type(), constant(mask, 8*sizeof(likely_type)))) : setType(b->CreateAnd(type(), constant(~mask, 8*sizeof(likely_type)))); }

    Value *depth() const { return get(likely_type_depth); }
    void setDepth(int depth) const { set(depth, likely_type_depth); }
    Value *isSigned() const { return get(likely_type_signed); }
    void setSigned(bool isSigned) const { setBit(isSigned, likely_type_signed); }
    Value *isFloating() const { return get(likely_type_floating); }
    void setFloating(bool isFloating) const { if (isFloating) setSigned(true); setBit(isFloating, likely_type_floating); }
    Value *isParallel() const { return get(likely_type_parallel); }
    void setParallel(bool isParallel) const { setBit(isParallel, likely_type_parallel); }
    Value *isHeterogeneous() const { return get(likely_type_heterogeneous); }
    void setHeterogeneous(bool isHeterogeneous) const { setBit(isHeterogeneous, likely_type_heterogeneous); }
    Value *isSingleChannel() const { return get(likely_type_single_channel); }
    void setSingleChannel(bool isSingleChannel) const { setBit(isSingleChannel, likely_type_single_channel); }
    Value *isSingleColumn() const { return get(likely_type_single_column); }
    void setSingleColumn(bool isSingleColumn) { setBit(isSingleColumn, likely_type_single_column); }
    Value *isSingleRow() const { return get(likely_type_single_row); }
    void setSingleRow(bool isSingleRow) const { setBit(isSingleRow, likely_type_single_row); }
    Value *isSingleFrame() const { return get(likely_type_single_frame); }
    void setSingleFrame(bool isSingleFrame) const { setBit(isSingleFrame, likely_type_single_frame); }
    Value *isSaturation() const { return get(likely_type_saturation); }
    void setSaturation(bool isSaturation) const { setBit(isSaturation, likely_type_saturation); }
    Value *reserved() const { return get(likely_type_reserved); }
    void setReserved(int reserved) const { set(reserved, likely_type_reserved); }

    Value *elements() const { return b->CreateMul(b->CreateMul(b->CreateMul(channels(), columns()), rows()), frames()); }
    Value *bytes() const { return b->CreateMul(b->CreateUDiv(b->CreateCast(Instruction::ZExt, depth(), Type::getInt32Ty(getGlobalContext())), constant(8, 32)), elements()); }

    Value *columnStep() const { Value *columnStep = channels(); columnStep->setName(n+"_cStep"); return columnStep; }
    Value *rowStep() const { return b->CreateMul(columns(), columnStep(), n+"_rStep"); }
    Value *frameStep() const { return b->CreateMul(rows(), rowStep(), n+"_tStep"); }

    Value *index(Value *c) const { return likely_single_channel(t) ? constant(0) : c; }
    Value *index(Value *c, Value *x) const { return likely_single_column(t) ? index(c) : b->CreateAdd(b->CreateMul(x, columnStep()), index(c)); }
    Value *index(Value *c, Value *x, Value *y) const { return likely_single_row(t) ? index(c, x) : b->CreateAdd(b->CreateMul(y, rowStep()), index(c, x)); }
    Value *index(Value *c, Value *x, Value *y, Value *f) const { return likely_single_frame(t) ? index(c, x, y) : b->CreateAdd(b->CreateMul(f, frameStep()), index(c, x, y)); }

    void deindex(Value *i, Value **c) const {
        *c = likely_single_channel(t) ? constant(0) : i;
    }
    void deindex(Value *i, Value **c, Value **x) const {
        Value *rem;
        if (likely_single_column(t)) {
            rem = i;
            *x = constant(0);
        } else {
            Value *step = columnStep();
            rem = b->CreateURem(i, step, n+"_xRem");
            *x = b->CreateExactUDiv(b->CreateSub(i, rem), step, n+"_x");
        }
        deindex(rem, c);
    }
    void deindex(Value *i, Value **c, Value **x, Value **y) const {
        Value *rem;
        if (likely_single_row(t)) {
            rem = i;
            *y = constant(0);
        } else {
            Value *step = rowStep();
            rem = b->CreateURem(i, step, n+"_yRem");
            *y = b->CreateExactUDiv(b->CreateSub(i, rem), step, n+"_y");
        }
        deindex(rem, c, x);
    }
    void deindex(Value *i, Value **c, Value **x, Value **y, Value **t_) const {
        Value *rem;
        if (likely_single_frame(t)) {
            rem = i;
            *t_ = constant(0);
        } else {
            Value *step = frameStep();
            rem = b->CreateURem(i, step, n+"_tRem");
            *t_ = b->CreateExactUDiv(b->CreateSub(i, rem), step, n+"_t");
        }
        deindex(rem, c, x, y);
    }

    LoadInst *load(Value *matrix, Type *type, Value *i) {
        LoadInst *load = b->CreateLoad(b->CreateGEP(data(matrix, type), i));
        load->setMetadata("llvm.mem.parallel_loop_access", getCurrentNode());
        return load;
    }
    LoadInst *load(Value *i) {
        LoadInst *load = b->CreateLoad(b->CreateGEP(data(), i));
        load->setMetadata("llvm.mem.parallel_loop_access", getCurrentNode());
        return load;
    }

    StoreInst *store(Value *matrix, Value *i, Value *value) {
        Value *d = data(matrix, ty(true));
        Value *idx = b->CreateGEP(d, i);
        StoreInst *store = b->CreateStore(value, idx);
        store->setMetadata("llvm.mem.parallel_loop_access", getCurrentNode());
        return store;
    }
    StoreInst *store(Value *i, Value *value) {
        StoreInst *store = b->CreateStore(value, b->CreateGEP(data(), i));
        store->setMetadata("llvm.mem.parallel_loop_access", getCurrentNode());
        return store;
    }

    Value *cast(Value *i, likely_type type) const { return ((t & likely_type_mask) == (type & likely_type_mask)) ? i : b->CreateCast(CastInst::getCastOpcode(i, likely_signed(t), Type::getFloatTy(getGlobalContext()), likely_signed(t)), i, Type::getFloatTy(getGlobalContext())); }

    // Saturation arithmetic logic:
    // http://locklessinc.com/articles/sat_arithmetic/
    Value *signedSaturationHelper(Value *result, Value *overflowResult, Value *overflowCondition) const
    {
        BasicBlock *overflowResolved = BasicBlock::Create(getGlobalContext(), n + "_overflow_resolved", f);
        BasicBlock *overflowTrue = BasicBlock::Create(getGlobalContext(), n + "_overflow_true", f);
        BasicBlock *overflowFalse = BasicBlock::Create(getGlobalContext(), n + "_overflow_false", f);
        b->CreateCondBr(overflowCondition, overflowTrue, overflowFalse);
        b->SetInsertPoint(overflowTrue);
        b->CreateBr(overflowResolved);
        b->SetInsertPoint(overflowFalse);
        b->CreateBr(overflowResolved);
        b->SetInsertPoint(overflowResolved);
        PHINode *conditionalResult = b->CreatePHI(result->getType(), 2);
        conditionalResult->addIncoming(overflowResult, overflowTrue);
        conditionalResult->addIncoming(result, overflowFalse);
        return conditionalResult;
    }

    Value *add(Value *i, Value *j) const
    {
        if (likely_floating(t)) {
            return b->CreateFAdd(i, j, n);
        } else {
            if (likely_saturation(t)) {
                if (likely_signed(t)) {
                    const int depth = likely_depth(t);
                    Value *result = b->CreateAdd(i, j, n);
                    Value *overflowResult = b->CreateAdd(b->CreateLShr(i, depth-1), intMax(depth));
                    Value *overflowCondition = b->CreateICmpSGE(b->CreateOr(b->CreateXor(i, j), b->CreateNot(b->CreateXor(j, result))), zero(depth));
                    return signedSaturationHelper(result, overflowResult, overflowCondition);
                } else {
                    Value *result = b->CreateAdd(i, j, n);
                    Value *overflow = b->CreateNeg(b->CreateZExt(b->CreateICmpULT(result, i, n),
                                                   Type::getIntNTy(getGlobalContext(), likely_depth(t))), n);
                    return b->CreateOr(result, overflow, n);
                }
            } else {
                return b->CreateAdd(i, j, n);
            }
        }
    }

    Value *subtract(Value *i, Value *j) const
    {
        if (likely_floating(t)) {
            return b->CreateFSub(i, j, n);
        } else {
            if (likely_saturation(t)) {
                if (likely_signed(t)) {
                    const int depth = likely_depth(t);
                    Value *result = b->CreateSub(i, j, n);
                    Value *overflowResult = b->CreateAdd(b->CreateLShr(i, depth-1), intMax(depth));
                    Value *overflowCondition = b->CreateICmpSLT(b->CreateAnd(b->CreateXor(i, j), b->CreateXor(i, result)), zero(depth));
                    return signedSaturationHelper(result, overflowResult, overflowCondition);
                } else {
                    Value *result = b->CreateSub(i, j, n);
                    Value *overflow = b->CreateNeg(b->CreateZExt(b->CreateICmpULE(result, i, n),
                                                   Type::getIntNTy(getGlobalContext(), likely_depth(t))), n);
                    return b->CreateAnd(result, overflow, n);
                }
            } else {
                return b->CreateSub(i, j, n);
            }
        }
    }

    Value *multiply(Value *i, Value *j) const
    {
        if (likely_floating(t)) {
            return b->CreateFMul(i, j, n);
        } else {
            if (likely_saturation(t)) {
                const int depth = likely_depth(t);
                Type *originalType = Type::getIntNTy(getGlobalContext(), depth);
                Type *extendedType = Type::getIntNTy(getGlobalContext(), 2*depth);
                Value *result = b->CreateMul(b->CreateZExt(i, extendedType),
                                             b->CreateZExt(j, extendedType), n);
                Value *lo = b->CreateTrunc(result, originalType, n);

                if (likely_signed(t)) {
                    Value *hi = b->CreateTrunc(b->CreateAShr(result, depth, n), originalType, n);
                    Value *overflowResult = b->CreateAdd(b->CreateLShr(b->CreateXor(i, j, n), depth-1, n), intMax(depth), n);
                    Value *overflowCondition = b->CreateICmpNE(hi, b->CreateAShr(lo, depth-1, n), n);
                    return signedSaturationHelper(b->CreateTrunc(result, i->getType(), n), overflowResult, overflowCondition);
                } else {
                    Value *hi = b->CreateTrunc(b->CreateLShr(result, depth, n), originalType, n);
                    Value *overflow = b->CreateNeg(b->CreateZExt(b->CreateICmpNE(hi, zero(depth), n), originalType, n), n);
                    return b->CreateOr(lo, overflow, n);
                }
            } else {
                return b->CreateMul(i, j, n);
            }
        }
    }

    Value *divide(Value *i, Value *j) const
    {
        if (likely_floating(t)) {
            return b->CreateFDiv(i, j, n);
        } else {
            if (likely_signed(t)) {
                if (likely_saturation(t)) {
                    const int depth = likely_depth(t);
                    Value *safe_i = b->CreateAdd(i, b->CreateZExt(b->CreateICmpNE(b->CreateOr(b->CreateAdd(j, constant(1, depth), n), b->CreateAdd(i, intMin(depth), n), n), zero(depth), n), i->getType(), n), n);
                    return b->CreateSDiv(safe_i, j, n);
                } else {
                    return b->CreateSDiv(i, j, n);
                }
            } else {
                return b->CreateUDiv(i, j, n);
            }
        }
    }

    Value *intrinsic(Value *i, Intrinsic::ID id) const { vector<Type*> args; args.push_back(i->getType()); Function *intrinsic = Intrinsic::getDeclaration(m, id, args); return b->CreateCall(intrinsic, i, n); }
    Value *log(Value *i) const { return intrinsic(i, Intrinsic::log); }
    Value *log2(Value *i) const { return intrinsic(i, Intrinsic::log2); }
    Value *log10(Value *i) const { return intrinsic(i, Intrinsic::log10); }
    Value *sin(Value *i) const { return intrinsic(i, Intrinsic::sin); }
    Value *cos(Value *i) const { return intrinsic(i, Intrinsic::cos); }
    Value *fabs(Value *i) const { return intrinsic(i, Intrinsic::fabs); }
    Value *sqrt(Value *i) const { return intrinsic(i, Intrinsic::sqrt); }
    Value *exp(Value *i) const { return intrinsic(i, Intrinsic::exp); }

    Value *compareLT(Value *i, Value *j) const { return likely_floating(t) ? b->CreateFCmpOLT(i, j) : (likely_signed(t) ? b->CreateICmpSLT(i, j) : b->CreateICmpULT(i, j)); }
    Value *compareGT(Value *i, Value *j) const { return likely_floating(t) ? b->CreateFCmpOGT(i, j) : (likely_signed(t) ? b->CreateICmpSGT(i, j) : b->CreateICmpUGT(i, j)); }

    Loop beginLoop(BasicBlock *entry, Value *start, Value *stop) {
        Loop loop;
        loop.stop = stop;
        loop.body = BasicBlock::Create(getGlobalContext(), n+"_loop_body", f);

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

        loop.i = b->CreatePHI(Type::getInt32Ty(getGlobalContext()), 2, n + "_i");
        loop.i->addIncoming(start, entry);

        loops.push(loop);
        return loop;
    }

    void endLoop() {
        const Loop &loop = loops.top();
        Value *increment = b->CreateAdd(loop.i, one(), n+"_loop_increment");
        BasicBlock *loopLatch = BasicBlock::Create(getGlobalContext(), n+"_loop_latch", f);
        b->CreateBr(loopLatch);
        b->SetInsertPoint(loopLatch);
        BasicBlock *loopExit = BasicBlock::Create(getGlobalContext(), n+"_loop_exit", f);
        BranchInst *latch = b->CreateCondBr(b->CreateICmpEQ(increment, loop.stop, n+"_loop_test"), loopExit, loop.body);
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
    inline Type *ty(bool pointer = false) const { return ty(t, pointer); }
    inline vector<Type*> tys(bool pointer = false) const { return toVector<Type*>(ty(pointer)); }

    static void *cleanup(Function *f) { f->removeFromParent(); delete f; return NULL; }
    void *cleanup() { return cleanup(f); }
};

class KernelBuilder
{
    string name;
    vector<string> stack;
    vector<likely_type> types;
    Module *m = NULL;
    ExecutionEngine *ee = NULL;
    TargetMachine *tm = NULL;

public:
    KernelBuilder(const string &name_, const vector<string> &tokens, const vector<likely_type> &types_)
        : name(name_), stack(tokens), types(types_)
    {
        m = new Module(name, getGlobalContext());
        likely_assert(m != NULL, "KernelBuilder failed to create LLVM Module");
        m->setTargetTriple(sys::getProcessTriple());

        string error;
        EngineBuilder engineBuilder(m);
        engineBuilder.setMCPU(sys::getHostCPUName());
        engineBuilder.setEngineKind(EngineKind::JIT);
        engineBuilder.setOptLevel(CodeGenOpt::Aggressive);
        engineBuilder.setErrorStr(&error);
        engineBuilder.setUseMCJIT(true);

        // Avoid an LLVM codegen bug on AVX architectures
        vector<string> mAttrs;
        mAttrs.push_back("-avx");
        engineBuilder.setMAttrs(mAttrs);

        ee = engineBuilder.create();
        likely_assert(ee != NULL, "KernelBuilder failed to create LLVM ExecutionEngine with error: %s", error.c_str());

        tm = engineBuilder.selectTarget();
        likely_assert(tm != NULL, "KernelBuilder failed to create LLVM TargetMachine with error: %s", error.c_str());
    }

    void *getPointerToFunction()
    {
        if (likely_parallel(types[0])) {
            Function *thunk = getFunction(name+"_thunk", m, types.size(), Type::getVoidTy(getGlobalContext()), PointerType::getUnqual(TheMatrixStruct), Type::getInt32Ty(getGlobalContext()), Type::getInt32Ty(getGlobalContext()));
            vector<Value*> thunkSrcs;
            getValues(thunk, thunkSrcs);
            Value *thunkStop = thunkSrcs.back(); thunkSrcs.pop_back();
            thunkStop->setName("stop");
            Value *thunkStart = thunkSrcs.back(); thunkSrcs.pop_back();
            thunkStart->setName("start");
            Value *thunkDst = thunkSrcs.back(); thunkSrcs.pop_back();
            thunkDst->setName("dst");
            BasicBlock *thunkEntry = BasicBlock::Create(getGlobalContext(), "thunk_entry", thunk);
            IRBuilder<> thunkBuilder(thunkEntry);
            MatrixBuilder thunkMatrix(m, &thunkBuilder, thunk, "thunk", types[0], thunkSrcs[0]);
            generateKernel(thunkMatrix, thunkEntry, thunkStart, thunkStop, thunkDst);
            thunkBuilder.CreateRetVoid();
        }

        Function *function = m->getFunction(name);
        if (function != NULL)
            return ee->getPointerToFunction(function);
        function = getFunction(name, m, types.size(), PointerType::getUnqual(TheMatrixStruct));

        vector<Value*> srcs;
        getValues(function, srcs);
        BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", function);
        IRBuilder<> builder(entry);
        MatrixBuilder matrix(m, &builder, function, "kernel", types[0], srcs[0]);

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
        Function *likelyNew = Function::Create(LikelyNewSignature, GlobalValue::ExternalLinkage, "likely_new", m);
        likelyNew->setCallingConv(CallingConv::C);
        likelyNew->setDoesNotAlias(0);
        likelyNew->setDoesNotAlias(6);
        likelyNew->setDoesNotCapture(6);

        std::vector<Value*> likelyNewArgs;
        likelyNewArgs.push_back(matrix.type());
        likelyNewArgs.push_back(matrix.channels());
        likelyNewArgs.push_back(matrix.columns());
        likelyNewArgs.push_back(matrix.rows());
        likelyNewArgs.push_back(matrix.frames());
        likelyNewArgs.push_back(ConstantPointerNull::get(Type::getInt8PtrTy(getGlobalContext())));
        likelyNewArgs.push_back(matrix.constant(0, 8));
        Value *dst = builder.CreateCall(likelyNew, likelyNewArgs);

        Value *start = MatrixBuilder::zero();
        Value *kernelSize = matrix.elements();
        if (likely_parallel(types[0])) {
            Function *thunk = getFunction(name+"_thunk", m, types.size(), Type::getVoidTy(getGlobalContext()), PointerType::getUnqual(TheMatrixStruct), Type::getInt32Ty(getGlobalContext()), Type::getInt32Ty(getGlobalContext()));
            vector<Value*> thunkSrcs;
            getValues(thunk, thunkSrcs);
            Value *thunkStop = thunkSrcs.back(); thunkSrcs.pop_back();
            thunkStop->setName("stop");
            Value *thunkStart = thunkSrcs.back(); thunkSrcs.pop_back();
            thunkStart->setName("start");
            Value *thunkDst = thunkSrcs.back(); thunkSrcs.pop_back();
            thunkDst->setName("dst");
            BasicBlock *thunkEntry = BasicBlock::Create(getGlobalContext(), "thunk_entry", thunk);
            IRBuilder<> thunkBuilder(thunkEntry);
            MatrixBuilder thunkMatrix(m, &thunkBuilder, thunk, "thunk", types[0], thunkSrcs[0]);
            generateKernel(thunkMatrix, thunkEntry, thunkStart, thunkStop, thunkDst);
            thunkBuilder.CreateRetVoid();

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
            Function *likelyFork = Function::Create(likelyForkType, GlobalValue::ExternalLinkage, "_likely_fork", m);
            likelyFork->setCallingConv(CallingConv::C);
            likelyFork->setDoesNotCapture(4);
            likelyFork->setDoesNotAlias(4);

            vector<Value*> likelyForkArgs;
            likelyForkArgs.push_back(m->getFunction(thunk->getName()));
            likelyForkArgs.push_back(MatrixBuilder::constant(types.size(), 8));
            likelyForkArgs.push_back(kernelSize);
            likelyForkArgs.insert(likelyForkArgs.end(), srcs.begin(), srcs.end());
            likelyForkArgs.push_back(dst);
            builder.CreateCall(likelyFork, likelyForkArgs);
        } else {
            generateKernel(matrix, entry, start, kernelSize, dst);
        }
        builder.CreateRet(dst);

        FunctionPassManager fpm(m);
        fpm.add(createVerifierPass(PrintMessageAction));
        tm->addAnalysisPasses(fpm);
        fpm.add(new TargetLibraryInfo(Triple(m->getTargetTriple())));
        fpm.add(new DataLayout(m));
        fpm.add(createBasicAliasAnalysisPass());
        fpm.add(createLICMPass());
        fpm.add(createLoopVectorizePass());
        fpm.add(createInstructionCombiningPass());
        fpm.add(createEarlyCSEPass());
        fpm.add(createCFGSimplificationPass());
        fpm.doInitialization();
//        DebugFlag = true;
        fpm.run(*function);
//        m->dump();

        ee->finalizeObject();
        return ee->getPointerToFunction(function);
    }

    static string interpret(lua_State *L)
    {
        const int args = lua_gettop(L);
        lua_likely_assert(L, lua_gettop(L) >= 1, "'interpret' expected one argument");

        // Make sure we were given a function
        lua_getfield(L, 1, "likely");
        lua_likely_assert(L, !strcmp("function", lua_tostring(L, -1)), "'compile' expected a function");
        lua_pop(L, 1);
        lua_getfield(L, 1, "name");
        const string name = lua_tostring(L, -1);
        lua_pop(L, 1);

        // Setup and call the function
        lua_getfield(L, 1, "source");
        lua_getfield(L, 1, "parameters");
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            lua_pushinteger(L, 3);
            lua_gettable(L, -2);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                lua_pushstring(L, "src"); // TODO: generalize
            }
            lua_insert(L, -4);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);

        // Get the function source code
        lua_call(L, lua_gettop(L)-args-1, 1);
        string source = lua_tostring(L, -1);
        lua_pop(L, 1);
        source.insert(0, "likely " + name + " ");
        return source;
    }

    static void getValues(Function *function, vector<Value*> &srcs)
    {
        Function::arg_iterator args = function->arg_begin();
        int i = 0;
        while (args != function->arg_end()) {
            Value *src = args++;
            stringstream name; name << "src" << char(int('A')+(i++));
            src->setName(name.str());
            srcs.push_back(src);
        }
    }

    static Function *getFunction(const string &name, Module *m, likely_arity arity, Type *ret, Type *dst = NULL, Type *start = NULL, Type *stop = NULL)
    {
        PointerType *matrixPointer = PointerType::getUnqual(TheMatrixStruct);
        Function *function;
        switch (arity) {
          case 0: function = cast<Function>(m->getOrInsertFunction(name, ret, dst, start, stop, NULL)); break;
          case 1: function = cast<Function>(m->getOrInsertFunction(name, ret, matrixPointer, dst, start, stop, NULL)); break;
          case 2: function = cast<Function>(m->getOrInsertFunction(name, ret, matrixPointer, matrixPointer, dst, start, stop, NULL)); break;
          case 3: function = cast<Function>(m->getOrInsertFunction(name, ret, matrixPointer, matrixPointer, matrixPointer, dst, start, stop, NULL)); break;
          default: { function = NULL; likely_assert(false, "KernelBuilder::getFunction invalid arity: %d", arity); }
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

private:
    bool generateKernel(MatrixBuilder &matrix, BasicBlock *entry, Value *start, Value *stop, Value *dst)
    {
        Value *i = matrix.beginLoop(entry, start, stop).i;

        vector<Value*> values;
        for (size_t j=0; j<stack.size(); j++) {
            const string &value = stack[j];
            char *error;
            const double x = strtod(value.c_str(), &error);
            if (*error == '\0') {
                values.push_back(matrix.autoConstant(x));
            } else if (value == "src") {
                values.push_back(matrix.load(i));
            } else if (values.size() == 1) {
                Value *operand = values[values.size()-1];
                values.pop_back();
                if      (value == "log")   values.push_back(matrix.log(operand));
                else if (value == "log2")  values.push_back(matrix.log2(operand));
                else if (value == "log10") values.push_back(matrix.log10(operand));
                else if (value == "sin")   values.push_back(matrix.sin(operand));
                else if (value == "cos")   values.push_back(matrix.cos(operand));
                else if (value == "fabs")  values.push_back(matrix.fabs(operand));
                else if (value == "sqrt")  values.push_back(matrix.sqrt(operand));
                else if (value == "exp")   values.push_back(matrix.exp(operand));
                else                       { likely_assert(false, "Unsupported unary operator: %s", value.c_str()); return false; }
            } else if (values.size() == 2) {
                Value *lhs = values[values.size()-2];
                Value *rhs = values[values.size()-1];
                values.pop_back();
                values.pop_back();
                if      (value == "+") values.push_back(matrix.add(lhs, rhs));
                else if (value == "-") values.push_back(matrix.subtract(lhs, rhs));
                else if (value == "*") values.push_back(matrix.multiply(lhs, rhs));
                else if (value == "/") values.push_back(matrix.divide(lhs, rhs));
                else                   { likely_assert(false, "Unsupported binary operator: %s", value.c_str()); return false; }
            } else {
                likely_assert(false, "Unrecognized token: %s", value.c_str()); return false;
            }
        }

        // Parsing a Reverse Polish Notation stack should yield one root value at the end
        if (values.size() != 1) {
            stringstream stream; stream << "[";
            for (size_t i=0; i<values.size(); i++) {
                stream << values[i];
                if (i < values.size()-1)
                    stream << ", ";
            }
            stream << "]";
            likely_assert(false, "Expected one value after parsing, got: %s", stream.str().c_str()); return false;
        }
        Value *equation = values[0];

        matrix.store(dst, i, equation);
        matrix.endLoop();
        return true;
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

static void executeWorker(int workerID)
{
    // There are hardware_concurrency-1 helper threads
    // The main thread which assumes workerID = workers.size()
    const likely_size step = (MaxRegisterWidth + (thunkSize-1)/(workers.size()+1)) / MaxRegisterWidth * MaxRegisterWidth;
    const likely_size start = workerID * step;
    const likely_size stop = std::min((workerID+1)*step, thunkSize);
    if (start >= stop) return;

    switch (thunkArity) {
      case 0: reinterpret_cast<likely_kernel_0>(currentThunk)((likely_mat)thunkMatricies[0], start, stop); break;
      case 1: reinterpret_cast<likely_kernel_1>(currentThunk)(thunkMatricies[0], (likely_mat)thunkMatricies[1], start, stop); break;
      case 2: reinterpret_cast<likely_kernel_2>(currentThunk)(thunkMatricies[0], thunkMatricies[1], (likely_mat)thunkMatricies[2], start, stop); break;
      case 3: reinterpret_cast<likely_kernel_3>(currentThunk)(thunkMatricies[0], thunkMatricies[1], thunkMatricies[2], (likely_mat)thunkMatricies[3], start, stop); break;
      default: likely_assert(false, "likely_parallel_dispatch invalid arity: %d", thunkArity);
    }
}

static void workerThread(int workerID)
{
    while (true) {
        workers[workerID]->lock();
        executeWorker(workerID);
        if (--workersRemaining == 0)
            workersActive.unlock();
    }
}

} // namespace likely

using namespace likely;

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

void *likely_compile_n(likely_description description, likely_arity n, likely_type *types)
{
    static map<string,KernelBuilder*> kernels;
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

        const int numWorkers = std::max((int)thread::hardware_concurrency()-1, 1);
        for (int i=0; i<numWorkers; i++) {
            mutex *m = new mutex();
            m->lock();
            workers.push_back(m);
            thread(workerThread, i).detach();
        }
    }

    const string prefix = "likely";
    string source = description;
    if (source.compare(0, prefix.size(), prefix)) {
        // It needs to be interpreted (usually the case)
        static lua_State *L = NULL;
        L = likely_exec((string("__likely =") + string(description)).c_str(), L);
        if (lua_type(L, -1) == LUA_TSTRING)
            likely_stack_dump(L);
        lua_getfield(L, -1, "__likely");
        lua_insert(L, 1);
        lua_settop(L, 1);
        source = KernelBuilder::interpret(L);
    }

    // Split source on space character
    vector<string> tokens;
    istringstream iss(source);
    copy(istream_iterator<string>(iss),
         istream_iterator<string>(),
         back_inserter< vector<string> >(tokens));
    tokens.erase(tokens.begin()); // remove "likely"

    // Compute function name and types
    stringstream nameStream; nameStream << tokens.front();
    tokens.erase(tokens.begin()); // remove name
    for (int i=0; i<n; i++) {
        likely_assert(types[i] != likely_type_null, "likely_compile_n null matrix at index: %d", i);
        nameStream << "_" << likely_type_to_string(types[i]);
    }
    const string name = nameStream.str();

    map<string,KernelBuilder*>::const_iterator it = kernels.find(name);
    if (it == kernels.end()) {
        kernels.insert(pair<string,KernelBuilder*>(name, new KernelBuilder(name, tokens, vector<likely_type>(types,types+n))));
        it = kernels.find(name);
    }
    return it->second->getPointerToFunction();
}

static int lua_likely_compile(lua_State *L)
{
    // Remove the matricies
    vector<likely_type> types;
    const int args = lua_gettop(L);
    for (int i=2; i<=args; i++)
        types.push_back(checkLuaMat(L, i)->type);

    // Get the intermediate representation
    const string source = KernelBuilder::interpret(L);

    // Compile the function
    void *compiled = likely_compile_n(source.c_str(), types.size(), types.data());
    lua_pushlightuserdata(L, compiled);
    return 1;
}

static int lua_likely_closure(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, (args >= 4) && (args <= 5), "'closure' expected 4-5 arguments, got: %d", args);

    lua_newtable(L);
    lua_pushstring(L, "likely");
    lua_pushstring(L, "function");
    lua_settable(L, -3);
    lua_pushstring(L, "name");
    lua_pushvalue(L, 1);
    lua_settable(L, -3);
    lua_pushstring(L, "source");
    lua_pushvalue(L, 2);
    lua_settable(L, -3);
    lua_pushstring(L, "documentation");
    lua_pushvalue(L, 3);
    lua_settable(L, -3);
    lua_pushstring(L, "parameters");
    lua_pushvalue(L, 4);
    lua_settable(L, -3);
    if (args >= 5) {
        lua_pushstring(L, "binary");
        lua_pushvalue(L, 5);
        lua_settable(L, -3);
    }

    lua_newtable(L);
    lua_pushnil(L);
    while (lua_next(L, 4)) {
        lua_pushinteger(L, 1);
        lua_gettable(L, -2);
        lua_pushvalue(L, -3);
        lua_settable(L, -5);
        lua_pop(L, 1);
    }
    lua_setfield(L, -2, "parameterLUT");

    luaL_getmetatable(L, "likely_function");
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_likely__call(lua_State *L)
{
    int args = lua_gettop(L);
    lua_likely_assert(L, args >= 1, "'__call' expected at least one argument");

    // Copy the arguments already in the closure...
    lua_newtable(L);
    lua_getfield(L, 1, "parameters");
    lua_pushnil(L);
    while (lua_next(L, -2)) {
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

    // ... and add the ones just provided
    if ((args == 2) && lua_istable(L, 2)) {
        // Special case, expand the table
        lua_getfield(L, 1, "parameterLUT");
        lua_pushnil(L);
        while (lua_next(L, 2)) {
            lua_pushvalue(L, -2);
            lua_gettable(L, -4);
            lua_gettable(L, -5);
            lua_insert(L, -2);
            lua_pushnumber(L, 3);
            lua_insert(L, -2);
            lua_settable(L, -3);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    } else {
        lua_pushnil(L);
        lua_next(L, -2);
        bool overrideArguments = false;
        for (int i=2; i<=args; i++) {
            // Find the next parameter without a bound argument
            while (!overrideArguments && (lua_objlen(L, -1) > 2)) {
                lua_pop(L, 1);
                if (!lua_next(L, -2)) {
                    if (i == 2) overrideArguments = true; // Override parameters if they all have values
                    if (!overrideArguments) luaL_error(L, "Too many arguments!");
                }
            }

            if (overrideArguments) {
                lua_pushinteger(L, i-1);
                lua_gettable(L, -2);
            }
            lua_pushinteger(L, 3);
            lua_pushvalue(L, i);
            lua_settable(L, -3);
            if (overrideArguments)
                lua_pop(L, 1);
        }
        if (!overrideArguments)
            lua_pop(L, 2);
    }

    // Are all the arguments provided?
    bool callable = true;
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        callable = callable && (lua_objlen(L, -1) >= 3);
        lua_pop(L, 1);
    }

    if (!callable) {
        // Return a new closure
        lua_getglobal(L, "closure");
        lua_getfield(L, 1, "name");
        lua_getfield(L, 1, "source");
        lua_getfield(L, 1, "documentation");
        lua_pushvalue(L, -5); // parameters
        lua_getfield(L, 1, "binary");
        lua_call(L, 5, 1);
        return 1;
    }

    // Compile the function if neeed
    lua_getfield(L, 1, "binary");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_getglobal(L, "likely");
        lua_getfield(L, -1, "compile");
        lua_pushvalue(L, 1);
        for (int i=2; i<=args; i++)
            lua_pushvalue(L, i);
        lua_call(L, args, 1);
    }

    if (lua_isuserdata(L, -1)) {
        // JIT function
        void *function = lua_touserdata(L, -1);
        vector<likely_const_mat> mats;
        for (int i=2; i<=args; i++)
            mats.push_back(checkLuaMat(L, i));

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

void _likely_fork(void *thunk, likely_arity arity, likely_size size, likely_const_mat src, ...)
{
    workersActive.lock();

    workersRemaining = workers.size();
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

    for (size_t i=0; i<workers.size(); i++)
        workers[i]->unlock();

    executeWorker(workers.size());
    while (workersRemaining > 0) {}
}

int luaopen_likely(lua_State *L)
{
    static const struct luaL_Reg likely_globals[] = {
        {"new", lua_likely_new},
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

    // Register function metatable
    luaL_newmetatable(L, "likely_function");
    lua_pushcfunction(L, lua_likely__call);
    lua_setfield(L, -2, "__call");

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
    typeFields.push_back(type_field("single_channel", likely_type_single_channel));
    typeFields.push_back(type_field("single_column", likely_type_single_column));
    typeFields.push_back(type_field("single_row", likely_type_single_row));
    typeFields.push_back(type_field("single_frame", likely_type_single_frame));
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
