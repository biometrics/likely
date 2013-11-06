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
#include <llvm/ExecutionEngine/JIT.h>
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

static Module *TheModule = NULL;
static StructType *TheMatrixStruct = NULL;
static const int MaxRegisterWidth = 32; // This should be determined at run time
static const int MostRecentErrorSize = 1024;
static char MostRecentError[MostRecentErrorSize];

static bool likelyAssertHelper(bool condition, const char *format, va_list ap, lua_State *L = NULL)
{
    if (condition) return true;
    vsnprintf(MostRecentError, MostRecentErrorSize, format, ap);
    if (L) luaL_error(L, "Likely %s.", MostRecentError);
    else   fprintf(stderr, "Likely %s.\n", MostRecentError);
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

const char *likely_most_recent_error()
{
    static string result;
    result = MostRecentError;
    sprintf(MostRecentError, "");
    return result.c_str();
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
    else if (!strcmp(field, "ref_count"))     lua_pushinteger(L, m->ref_count);
    else if (!strcmp(field, "depth"))         lua_pushinteger(L, likely_depth(m->type));
    else if (!strcmp(field, "signed"))        lua_pushboolean(L, likely_signed(m->type));
    else if (!strcmp(field, "floating"))      lua_pushboolean(L, likely_floating(m->type));
    else if (!strcmp(field, "parallel"))      lua_pushboolean(L, likely_parallel(m->type));
    else if (!strcmp(field, "heterogeneous")) lua_pushboolean(L, likely_heterogeneous(m->type));
    else if (!strcmp(field, "singleChannel")) lua_pushboolean(L, likely_single_channel(m->type));
    else if (!strcmp(field, "singleColumn"))  lua_pushboolean(L, likely_single_column(m->type));
    else if (!strcmp(field, "singleRow"))     lua_pushboolean(L, likely_single_row(m->type));
    else if (!strcmp(field, "singleFrame"))   lua_pushboolean(L, likely_single_frame(m->type));
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
    else if (!strcmp(field, "ref_count"))   { m->ref_count = lua_tointegerx(L, 3, &isnum); likely_assert(isnum, "'set' expected ref_count to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "depth"))       { likely_set_depth(&m->type, lua_tointegerx(L, 3, &isnum)); likely_assert(isnum, "'set' expected depth to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "signed"))        likely_set_signed(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "floating"))      likely_set_floating(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "parallel"))      likely_set_parallel(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "heterogeneous")) likely_set_heterogeneous(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "singleChannel")) likely_set_single_channel(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "singleColumn"))  likely_set_single_column(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "singleRow"))     likely_set_single_row(&m->type, lua_toboolean(L, 3));
    else if (!strcmp(field, "singleFrame"))   likely_set_single_frame(&m->type, lua_toboolean(L, 3));
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
static size_t recycledDataBytes = 0;

likely_mat likely_new(likely_type type, likely_size channels, likely_size columns, likely_size rows, likely_size frames, likely_data *data, int8_t copy)
{
    likely_mat dst;
    const size_t dataBytes =  uint64_t(likely_depth(type)) * uint64_t((data && !copy) ? 0 : channels*columns*rows*frames) / uint64_t(8);
    const size_t headerBytes = sizeof(likely_matrix) + (MaxRegisterWidth - 1);
    if (recycledBuffer) {
        if (recycledDataBytes >= dataBytes) dst = recycledBuffer;
        else                                dst = (likely_mat) realloc(recycledBuffer, headerBytes + dataBytes);
        recycledBuffer = NULL;
    } else {
        dst = (likely_mat) malloc(headerBytes + dataBytes);
    }
    dst->data = reinterpret_cast<likely_data*>((uintptr_t(dst+1)+(MaxRegisterWidth-1)) & ~uintptr_t(MaxRegisterWidth-1));
    dst->type = type;
    dst->channels = channels;
    dst->columns = columns;
    dst->rows = rows;
    dst->frames = frames;
    dst->ref_count = 1;

    if (data) {
        if (copy) memcpy(dst->data, data, likely_bytes(dst));
        else      dst->data = data;
    }
    return dst;
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

likely_mat likely_retain(likely_mat m)
{
    if (m && m->ref_count)
        m->ref_count++;
    return m;
}

static int lua_likely_retain(lua_State *L)
{
    likely_assert(lua_gettop(L) == 1, "'retain' expected 1 argument, got: %d", lua_gettop(L));
    likely_retain(checkLuaMat(L));
    lua_pushvalue(L, -1);
    return 1;
}

void likely_release(likely_mat m)
{
    if (!m || !m->ref_count) return;
    m->ref_count--;
    if (m->ref_count > 0) return;
    const size_t dataBytes = likely_bytes(m);
    if (recycledBuffer) {
        if (dataBytes > recycledDataBytes) {
            free(recycledBuffer);
            recycledBuffer = m;
            recycledDataBytes = dataBytes;
        } else {
            free(m);
        }
    } else {
        recycledBuffer = m;
        recycledDataBytes = dataBytes;
    }
}

static int lua_likely_release(lua_State *L)
{
    likely_assert(lua_gettop(L) == 1, "'release' expected 1 argument, got: %d", lua_gettop(L));
    likely_release(checkLuaMat(L));
    return 0;
}

static likely_mat likelyReadHelper(const char *fileName, lua_State *L = NULL)
{
    static string previousFileName;
    static likely_mat previousMat = NULL;
    if (previousFileName == fileName)
        return previousMat;

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

likely_mat likely_render(likely_const_mat m)
{
    if ((likely_depth(m->type) == 8) && !likely_floating(m->type) && (m->channels == 3)) {
        likely_mat n = const_cast<likely_mat>(m); // We don't consider a call to retain as violating logical constness
        likely_retain(n);
        return n;
    }

    likely_mat n = likely_new(likely_type_u8, 3, m->columns, m->rows);
    for (likely_size y=0; y<n->rows; y++) {
        for (likely_size x=0; x<n->columns; x++) {
            for (likely_size c=0; c<3; c++) {
                const double value = likely_element(m, c % m->channels, x, y);
                likely_set_element(n, value, c, x, y);
            }
        }
    }
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

void likely_dump()
{
    TheModule->dump();
}

static lua_State *getLuaState()
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaL_requiref(L, "likely", luaopen_likely, 1);
    lua_pop(L, 1);
    likely_stack_dump(L, luaL_dostring(L, likely_standard_library()));
    return L;
}

namespace likely
{

struct MatrixBuilder
{
    IRBuilder<> *b;
    Function *f;
    Twine n;
    likely_type t;
    Value *v;

    struct Loop {
        BasicBlock *body, *exit;
        PHINode *i;
        Value *stop;
        MDNode *node;
    };
    stack<Loop> loops;

    MatrixBuilder() : b(NULL), f(NULL), t(likely_type_null), v(NULL) {}
    MatrixBuilder(IRBuilder<> *builder, Function *function, const Twine &name, likely_type type = likely_type_null, Value *value = NULL)
        : b(builder), f(function), n(name), t(type), v(value) {}
    void reset(IRBuilder<> *builder, Function *function, Value *value) { b = builder; f = function; v = value; }

    static Constant *constant(int value, int bits = 32) { return Constant::getIntegerValue(Type::getInt32Ty(getGlobalContext()), APInt(bits, value)); }
    static Constant *constant(bool value) { return constant(value, 1); }
    static Constant *constant(float value) { return ConstantFP::get(Type::getFloatTy(getGlobalContext()), value == 0 ? -0.0f : value); }
    static Constant *constant(double value) { return ConstantFP::get(Type::getDoubleTy(getGlobalContext()), value == 0 ? -0.0 : value); }
    static Constant *constant(const char *value) { return ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(getGlobalContext(), 8*sizeof(value)), uint64_t(value)), Type::getInt8PtrTy(getGlobalContext())); }
    template <typename T>
    static Constant *constant(T value, Type *type) { return ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(getGlobalContext(), 8*sizeof(value)), uint64_t(value)), type); }
    static Constant *zero() { return constant(0); }
    static Constant *one() { return constant(1); }
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

    Value *newMat() const
    {
        static Function *likely_new = NULL;
        if (likely_new == NULL) {
            Type *newReturn = PointerType::getUnqual(TheMatrixStruct);
            vector<Type*> newParameters;
            newParameters.push_back(Type::getInt32Ty(getGlobalContext())); // type
            newParameters.push_back(Type::getInt32Ty(getGlobalContext())); // channels
            newParameters.push_back(Type::getInt32Ty(getGlobalContext())); // columns
            newParameters.push_back(Type::getInt32Ty(getGlobalContext())); // rows
            newParameters.push_back(Type::getInt32Ty(getGlobalContext())); // frames
            newParameters.push_back(Type::getInt8PtrTy(getGlobalContext())); // data
            newParameters.push_back(Type::getInt8Ty(getGlobalContext())); // copy
            FunctionType* elementsType = FunctionType::get(newReturn, newParameters, false);
            likely_new = Function::Create(elementsType, GlobalValue::ExternalLinkage, "likely_new", TheModule);
            likely_new->setCallingConv(CallingConv::C);
        }
        std::vector<Value*> args;
        args.push_back(type());
        args.push_back(channels());
        args.push_back(columns());
        args.push_back(rows());
        args.push_back(frames());
        args.push_back(ConstantPointerNull::get(Type::getInt8PtrTy(getGlobalContext())));
        args.push_back(constant(1, 8));
        return b->CreateCall(likely_new, args);
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
    Value *add(Value *i, Value *j) const { return likely_floating(t) ? b->CreateFAdd(i, j, n) : b->CreateAdd(i, j, n); }
    Value *subtract(Value *i, Value *j) const { return likely_floating(t) ? b->CreateFSub(i, j, n) : b->CreateSub(i, j, n); }
    Value *multiply(Value *i, Value *j) const { return likely_floating(t) ? b->CreateFMul(i, j, n) : b->CreateMul(i, j, n); }
    Value *divide(Value *i, Value *j) const { return likely_floating(t) ? b->CreateFDiv(i, j, n) : (likely_signed(t) ? b->CreateSDiv(i,j, n) : b->CreateUDiv(i, j, n)); }

    Value *intrinsic(Value *i, Intrinsic::ID id) const { vector<Type*> args; args.push_back(i->getType()); Function *intrinsic = Intrinsic::getDeclaration(TheModule, id, args); return b->CreateCall(intrinsic, i, n); }
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
        loop.exit = BasicBlock::Create(getGlobalContext(), n+"_loop_exit", f);

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

        loop.i = b->CreatePHI(Type::getInt32Ty(getGlobalContext()), 2, n);
        loop.i->addIncoming(start, entry);

        loops.push(loop);
        return loop;
    }

    void endLoop() {
        const Loop &loop = loops.top();
        Value *increment = b->CreateAdd(loop.i, one(), n+"_loop_increment");
        loop.i->addIncoming(increment, loop.body);

        BranchInst *latch = b->CreateCondBr(b->CreateICmpEQ(increment, loop.stop, n+"_loop_test"), loop.exit, loop.body);
        latch->setMetadata("llvm.loop.parallel", loop.node);

        b->SetInsertPoint(loop.exit);
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
    likely_description description;
    vector<string> stack;
    vector<likely_type> types;
    MatrixBuilder kernel;
    PHINode *i = NULL;

public:
    KernelBuilder() = default;
    KernelBuilder(likely_description description_)
        : description(description_)
    {
        const string prefix = "likely";
        string source = description;
        if (source.compare(0, prefix.size(), prefix)) {
            // It needs to be interpreted
            lua_State *L = getLuaState();
            likely_stack_dump(L, luaL_dostring(L, (string("return ") + description).c_str()));
            source = interpret(L);
            lua_settop(L, 0);
        }

        // Split on space character
        istringstream iss(source);
        copy(istream_iterator<string>(iss),
             istream_iterator<string>(),
             back_inserter< vector<string> >(stack));
        stack.erase(stack.begin()); // remove "likely"
    }

    static string interpret(lua_State *L)
    {
        const int args = lua_gettop(L);
        lua_likely_assert(L, lua_gettop(L) >= 1, "'interpret' expected one argument");

        // Make sure we were given a function
        lua_getfield(L, 1, "likely");
        lua_likely_assert(L, !strcmp("function", lua_tostring(L, -1)), "'compile' expected a function");
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
        source.insert(0, "likely ");
        return source;
    }

    bool makeAllocation(Function *function, const vector<likely_type> &types_)
    {
        types = types_;
        vector<Value*> srcs;
        getValues(function, srcs);

        BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", function);
        IRBuilder<> builder(entry);

        likely_type kernelType = likely_type_null;
        for (likely_type type : types)
            kernelType |= type;

        kernel = MatrixBuilder(&builder, function, "kernel", kernelType, srcs[0]);
        builder.CreateRet(kernel.newMat());
        return true;
    }

    bool makeKernel(Function *function)
    {
        vector<Value*> srcs;
        Value *dst, *start, *stop;
        getValues(function, srcs, dst, start, stop);

        function->addFnAttr(Attribute::NoUnwind);
        for (size_t i=1; i<function->arg_size()-1; i++) { // Exclude kernel start and stop arguments
            function->setDoesNotAlias(i);
            function->setDoesNotCapture(i);
        }

        BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", function);
        IRBuilder<> builder(entry);

        if (likely_parallel(kernel.t)) {
            vector<likely_mat> mats;
            for (likely_type type : types) {
                likely_mat m = likely_new(type);
                likely_set_parallel(&m->type, false);
                mats.push_back(m);
            }

            // Allocation must be called first to properly initialize the kernel builder
            likely_compile_allocation_n(description, mats.size(), (likely_const_mat*)mats.data());
            void *serialKernelFunction = likely_compile_kernel_n(description, mats.size(), (likely_const_mat*)mats.data());

            static Function *parallelDispatch = NULL;
            if (parallelDispatch == NULL) {
                vector<Type*> params;
                params.push_back(Type::getInt8PtrTy(getGlobalContext()));
                params.push_back(Type::getInt8Ty(getGlobalContext()));
                params.push_back(Type::getInt32Ty(getGlobalContext()));
                params.push_back(Type::getInt32Ty(getGlobalContext()));
                params.push_back(PointerType::getUnqual(TheMatrixStruct));
                parallelDispatch = Function::Create(FunctionType::get(Type::getVoidTy(getGlobalContext()), params, true), GlobalValue::ExternalLinkage, "likely_parallel_dispatch", TheModule);
                parallelDispatch->setCallingConv(CallingConv::C);
            }

            vector<Value*> args;
            args.push_back(MatrixBuilder::constant(reinterpret_cast<const char*>(serialKernelFunction)));
            args.push_back(MatrixBuilder::constant(mats.size(), 8));
            args.push_back(start);
            args.push_back(stop);
            args.insert(args.end(), srcs.begin(), srcs.end());
            args.push_back(dst);

            builder.CreateCall(parallelDispatch, args);

            for (likely_mat m : mats)
                likely_release(m);
        } else {
            kernel.reset(&builder, function, srcs[0]);
            i = kernel.beginLoop(entry, start, stop).i;
            Value *equation = makeEquation();
            if (!equation) return false;
            kernel.store(dst, i, makeEquation());
            kernel.endLoop();
        }

        builder.CreateRetVoid();
        return true;
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

    static void getValues(Function *function, vector<Value*> &srcs, Value *&dst, Value *&start, Value *&stop)
    {
        getValues(function, srcs);
        stop = srcs.back(); srcs.pop_back();
        stop->setName("stop");
        start = srcs.back(); srcs.pop_back();
        start->setName("start");
        dst = srcs.back(); srcs.pop_back();
        dst->setName("dst");
    }

    Value *makeEquation()
    {
        vector<Value*> values;
        for (size_t j=0; j<stack.size(); j++) {
            const string &value = stack[j];
            char *error;
            const double x = strtod(value.c_str(), &error);
            if (*error == '\0') {
                values.push_back(kernel.autoConstant(x));
            } else if (value == "src") {
                values.push_back(kernel.load(i));
            } else if (values.size() == 1) {
                Value *operand = values[values.size()-1];
                values.pop_back();
                if      (value == "log")   values.push_back(kernel.log(operand));
                else if (value == "log2")  values.push_back(kernel.log2(operand));
                else if (value == "log10") values.push_back(kernel.log10(operand));
                else if (value == "sin")   values.push_back(kernel.sin(operand));
                else if (value == "cos")   values.push_back(kernel.cos(operand));
                else if (value == "fabs")  values.push_back(kernel.fabs(operand));
                else if (value == "sqrt")  values.push_back(kernel.sqrt(operand));
                else if (value == "exp")   values.push_back(kernel.exp(operand));
                else                       { likely_assert(false, "Unsupported unary operator: %s", value.c_str()); return NULL; }
            } else if (values.size() == 2) {
                Value *lhs = values[values.size()-2];
                Value *rhs = values[values.size()-1];
                values.pop_back();
                values.pop_back();
                if      (value == "+") values.push_back(kernel.add(lhs, rhs));
                else if (value == "-") values.push_back(kernel.subtract(lhs, rhs));
                else if (value == "*") values.push_back(kernel.multiply(lhs, rhs));
                else if (value == "/") values.push_back(kernel.divide(lhs, rhs));
                else                   { likely_assert(false, "Unsupported binary operator: %s", value.c_str()); return NULL; }
            } else {
                likely_assert(false, "Unrecognized token: %s", value.c_str()); return NULL;
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
            likely_assert(false, "Expected one value after parsing, got: %s", stream.str().c_str()); return NULL;
        }
        return values[0];
    }
};

static vector<likely_description> descriptions;
static map<string,KernelBuilder> kernels;
static ExecutionEngine *executionEngine = NULL;
static TargetMachine *targetMachine = NULL;
static recursive_mutex makerLock;

// Control parallel execution
static vector<mutex*> workers;
static mutex workersActive;
static atomic_uint workersRemaining(0);
static void *workerKernel = NULL;
static likely_arity workerArity = 0;
static likely_size workerStart = 0;
static likely_size workerStop = 0;
static likely_mat workerMatricies[LIKELY_NUM_ARITIES+1];

static string mangledName(const string &description, const vector<likely_type> &types)
{
    stringstream stream; stream << description;
    for (likely_type type : types)
        stream << "_" << likely_type_to_string(type);
    return stream.str();
}

static Function *getFunction(const string &description, likely_arity arity, Type *ret, Type *dst = NULL, Type *start = NULL, Type *stop = NULL)
{
    PointerType *matrixPointer = PointerType::getUnqual(TheMatrixStruct);
    Function *function;
    switch (arity) {
      case 0: function = cast<Function>(TheModule->getOrInsertFunction(description, ret, dst, start, stop, NULL)); break;
      case 1: function = cast<Function>(TheModule->getOrInsertFunction(description, ret, matrixPointer, dst, start, stop, NULL)); break;
      case 2: function = cast<Function>(TheModule->getOrInsertFunction(description, ret, matrixPointer, matrixPointer, dst, start, stop, NULL)); break;
      case 3: function = cast<Function>(TheModule->getOrInsertFunction(description, ret, matrixPointer, matrixPointer, matrixPointer, dst, start, stop, NULL)); break;
      default: function = NULL;
    }
    likely_assert(function, "FunctionBuilder::getFunction invalid arity: %d", arity);
    function->setCallingConv(CallingConv::C);
    return function;
}

static void executeWorker(int workerID)
{
    // There are hardware_concurrency-1 helper threads
    // The main thread which assumes workerID = workers.size()
    const likely_size step = (MaxRegisterWidth + (workerStop-workerStart-1)/(workers.size()+1)) / MaxRegisterWidth * MaxRegisterWidth;
    const likely_size start = workerStart + workerID * step;
    const likely_size stop = std::min(workerStart + (workerID+1)*step, workerStop);
    if (start >= stop) return;

    switch (workerArity) {
      case 0: reinterpret_cast<likely_kernel_0>(workerKernel)(workerMatricies[0], start, stop); break;
      case 1: reinterpret_cast<likely_kernel_1>(workerKernel)(workerMatricies[0], workerMatricies[1], start, stop); break;
      case 2: reinterpret_cast<likely_kernel_2>(workerKernel)(workerMatricies[0], workerMatricies[1], workerMatricies[2], start, stop); break;
      case 3: reinterpret_cast<likely_kernel_3>(workerKernel)(workerMatricies[0], workerMatricies[1], workerMatricies[2], workerMatricies[3], start, stop); break;
      default: likely_assert(false, "likely_parallel_dispatch invalid arity: %d", workerArity);
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

void *likely_compile(likely_description description, likely_arity n, likely_const_mat src, ...)
{
    vector<likely_const_mat> srcs;
    va_list ap;
    va_start(ap, src);
    for (int i=0; i<n; i++) {
        srcs.push_back(src);
        src = va_arg(ap, likely_const_mat);
    }
    va_end(ap);
    return likely_compile_n(description, n, srcs.data());
}

void *likely_compile_n(likely_description description, likely_arity n, likely_const_mat *srcList)
{
    lock_guard<recursive_mutex> lock(makerLock);

    if (TheModule == NULL) {
        // Initialize Likely
        InitializeNativeTarget();
        TheModule = new Module("likely", getGlobalContext());
        TheModule->setTargetTriple(sys::getProcessTriple());

        string error;
        EngineBuilder engineBuilder(TheModule);
        engineBuilder.setMCPU(sys::getHostCPUName());
        engineBuilder.setEngineKind(EngineKind::JIT);
        engineBuilder.setOptLevel(CodeGenOpt::Aggressive);
        engineBuilder.setErrorStr(&error);

        executionEngine = engineBuilder.create();
        likely_assert(executionEngine != NULL, "likely_compile_n failed to create LLVM ExecutionEngine with error: %s", error.c_str());

        targetMachine = engineBuilder.selectTarget();
        likely_assert(targetMachine != NULL, "likely_compile_n failed to create LLVM TargetMachine with error: %s", error.c_str());

        TheMatrixStruct = StructType::create("Matrix",
                                             Type::getInt8PtrTy(getGlobalContext()), // data
                                             Type::getInt32Ty(getGlobalContext()),   // type
                                             Type::getInt32Ty(getGlobalContext()),   // channels
                                             Type::getInt32Ty(getGlobalContext()),   // columns
                                             Type::getInt32Ty(getGlobalContext()),   // rows
                                             Type::getInt32Ty(getGlobalContext()),   // frames
                                             Type::getInt32Ty(getGlobalContext()),   // ref_count
                                             NULL);

        const int numWorkers = std::max((int)thread::hardware_concurrency()-1, 1);
        for (int i=0; i<numWorkers; i++) {
            mutex *m = new mutex();
            m->lock();
            workers.push_back(m);
            thread(workerThread, i).detach();
        }
    }

    Function *function = TheModule->getFunction(description);
    if (function != NULL)
        return executionEngine->getPointerToFunction(function);
    function = getFunction(description, n, PointerType::getUnqual(TheMatrixStruct));

    static vector<Type*> makerParameters;
    if (makerParameters.empty()) {
        makerParameters.push_back(Type::getInt8PtrTy(getGlobalContext()));
        makerParameters.push_back(Type::getInt8Ty(getGlobalContext()));
        makerParameters.push_back(PointerType::getUnqual(TheMatrixStruct));
    }

    static Function *makeAllocationFunction = NULL;
    static vector<PointerType*> allocationFunctionTypes(LIKELY_NUM_ARITIES, NULL);
    PointerType *allocationFunctionType = allocationFunctionTypes[n];
    if (allocationFunctionType == NULL) {
        Type *allocationReturn = PointerType::getUnqual(TheMatrixStruct);
        vector<Type*> allocationParams;
        for (int i=0; i<n; i++)
            allocationParams.push_back(PointerType::getUnqual(TheMatrixStruct));
        allocationFunctionType = PointerType::getUnqual(FunctionType::get(allocationReturn, allocationParams, false));
        allocationFunctionTypes[n] = allocationFunctionType;

        if (makeAllocationFunction == NULL) {
            FunctionType* makeAllocationType = FunctionType::get(allocationFunctionType, makerParameters, true);
            makeAllocationFunction = Function::Create(makeAllocationType, GlobalValue::ExternalLinkage, "likely_compile_allocation", TheModule);
            makeAllocationFunction->setCallingConv(CallingConv::C);
        }
    }
    GlobalVariable *allocationFunction = cast<GlobalVariable>(TheModule->getOrInsertGlobal(string(description)+"_allocation", allocationFunctionType));
    void *default_allocation = likely_compile_allocation_n(description, n, srcList);
    if (default_allocation == NULL) return MatrixBuilder::cleanup(function);
    allocationFunction->setInitializer(MatrixBuilder::constant<void*>(default_allocation, allocationFunctionType));

    static Function *makeKernelFunction = NULL;
    static vector<PointerType*> kernelFunctionTypes(LIKELY_NUM_ARITIES, NULL);
    PointerType *kernelFunctionType = kernelFunctionTypes[n];
    if (kernelFunctionType == NULL) {
        Type *kernelReturn = Type::getVoidTy(getGlobalContext());
        vector<Type*> kernelParams;
        for (int i=0; i<n+1; i++)
            kernelParams.push_back(PointerType::getUnqual(TheMatrixStruct));
        kernelParams.push_back(Type::getInt32Ty(getGlobalContext()));
        kernelParams.push_back(Type::getInt32Ty(getGlobalContext()));
        kernelFunctionType = PointerType::getUnqual(FunctionType::get(kernelReturn, kernelParams, false));
        kernelFunctionTypes[n] = kernelFunctionType;

        if (makeKernelFunction == NULL) {
            FunctionType* makeUnaryKernelType = FunctionType::get(kernelFunctionType, makerParameters, true);
            makeKernelFunction = Function::Create(makeUnaryKernelType, GlobalValue::ExternalLinkage, "likely_compile_kernel", TheModule);
            makeKernelFunction->setCallingConv(CallingConv::C);
        }
    }
    GlobalVariable *kernelFunction = cast<GlobalVariable>(TheModule->getOrInsertGlobal(string(description)+"_kernel", kernelFunctionType));
    void *default_kernel = likely_compile_kernel_n(description, n, srcList);
    if (default_kernel == NULL) return MatrixBuilder::cleanup(function);
    kernelFunction->setInitializer(MatrixBuilder::constant<void*>(default_kernel, kernelFunctionType));

    vector<Value*> srcs;
    KernelBuilder::getValues(function, srcs);

    BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", function);
    IRBuilder<> builder(entry);

    vector<GlobalVariable*> kernelTypes;
    for (int i=0; i<n; i++) {
        GlobalVariable *kernelType = cast<GlobalVariable>(TheModule->getOrInsertGlobal(string(description)+"_type"+to_string(i), Type::getInt32Ty(getGlobalContext())));
        kernelType->setInitializer(MatrixBuilder::constant(srcList[i]->type, 8*sizeof(likely_type)));
        kernelTypes.push_back(kernelType);
    }

    vector<Value*> srcTypes;
    for (int i=0; i<n; i++)
        srcTypes.push_back(builder.CreateLoad(builder.CreateStructGEP(srcs[i], 1), "src_type"+to_string(i)));

    Value *typeTest = MatrixBuilder::constant(true);
    for (int i=0; i<n; i++)
        typeTest = builder.CreateAnd(typeTest, builder.CreateICmpEQ(builder.CreateLoad(kernelTypes[i]), srcTypes[i]));

    BasicBlock *typeFail = BasicBlock::Create(getGlobalContext(), "type_fail", function);
    BasicBlock *execute = BasicBlock::Create(getGlobalContext(), "execute", function);
    builder.CreateCondBr(typeTest, execute, typeFail);

    builder.SetInsertPoint(typeFail);
    {
        // Construct a description that stays valid for the lifetime of the program
        char *copy = new char[strlen(description)+1];
        strcpy(copy, description);
        descriptions.push_back(copy);

        vector<Value*> args;
        args.push_back(MatrixBuilder::constant(copy));
        args.push_back(MatrixBuilder::constant(n, 8));
        args.insert(args.end(), srcs.begin(), srcs.end());
        builder.CreateStore(builder.CreateCall(makeAllocationFunction, args), allocationFunction);
        builder.CreateStore(builder.CreateCall(makeKernelFunction, args), kernelFunction);
        for (int i=0; i<n; i++)
            builder.CreateStore(srcTypes[i], kernelTypes[i]);
        builder.CreateBr(execute);
    }

    builder.SetInsertPoint(execute);
    {
        vector<Value*> args(srcs);
        Value *dst = builder.CreateCall(builder.CreateLoad(allocationFunction), args);

        static Function *likely_elements = NULL;
        if (likely_elements == NULL) {
            Type *elementsReturn = Type::getInt32Ty(getGlobalContext());
            vector<Type*> elementsParameters;
            elementsParameters.push_back(PointerType::getUnqual(TheMatrixStruct));
            FunctionType* elementsType = FunctionType::get(elementsReturn, elementsParameters, false);
            likely_elements = Function::Create(elementsType, GlobalValue::ExternalLinkage, "likely_elements", TheModule);
            likely_elements->setCallingConv(CallingConv::C);
        }
        Value *kernelSize = builder.CreateCall(likely_elements, dst);

        BasicBlock *kernel = BasicBlock::Create(getGlobalContext(), "kernel", function);
        BasicBlock *exit = BasicBlock::Create(getGlobalContext(), "exit", function);
        builder.CreateCondBr(builder.CreateICmpUGT(kernelSize, MatrixBuilder::zero()), kernel, exit);

        builder.SetInsertPoint(kernel);
        args.push_back(dst);
        args.push_back(MatrixBuilder::zero());
        args.push_back(kernelSize);
        builder.CreateCall(builder.CreateLoad(kernelFunction), args);
        builder.CreateBr(exit);

        builder.SetInsertPoint(exit);
        builder.CreateRet(dst);
    }

    static FunctionPassManager *functionPassManager = NULL;
    if (functionPassManager == NULL) {
        functionPassManager = new FunctionPassManager(TheModule);
        functionPassManager->add(createVerifierPass(PrintMessageAction));
    }
    functionPassManager->run(*function);

    return executionEngine->getPointerToFunction(function);
}

static int lua_likely_compile(lua_State *L)
{
    // Remove the matricies
    vector<likely_const_mat> mats;
    const int args = lua_gettop(L);
    for (int i=2; i<=args; i++)
        mats.push_back(checkLuaMat(L, i));

    // Get the intermediate representation
    const string source = KernelBuilder::interpret(L);

    // Compile the function
    void *compiled = likely_compile_n(source.c_str(), mats.size(), mats.data());
    lua_pushlightuserdata(L, compiled);
    return 1;
}

static int lua_likely_closure(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, (args >= 3) && (args <= 4), "'closure' expected 3-4 arguments, got: %d", args);

    lua_newtable(L);
    lua_pushstring(L, "likely");
    lua_pushstring(L, "function");
    lua_settable(L, -3);
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
    while (lua_next(L, 3)) {
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
        lua_getfield(L, 1, "source");
        lua_getfield(L, 1, "documentation");
        lua_pushvalue(L, -4); // parameters
        lua_getfield(L, 1, "binary");
        lua_call(L, 4, 1);
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

void *likely_compile_allocation(likely_description description, likely_arity n, likely_const_mat src, ...)
{
    vector<likely_const_mat> srcs;
    va_list ap;
    va_start(ap, src);
    for (int i=0; i<n; i++) {
        srcs.push_back(src);
        src = va_arg(ap, likely_const_mat);
    }
    va_end(ap);
    return likely_compile_allocation_n(description, n, srcs.data());
}

void *likely_compile_allocation_n(likely_description description, likely_arity n, likely_const_mat *srcs)
{
    vector<likely_type> types;
    for (int i=0; i<n; i++) {
        likely_const_mat src = srcs[i];
        likely_assert(src && (src->type != likely_type_null), "likely_compile_allocation_n null matrix at index: %d", i);
        types.push_back(src->type);
    }

    lock_guard<recursive_mutex> lock(makerLock);
    const string name = mangledName(description, types)+"_allocation";

    Function *function = TheModule->getFunction(name);
    if (function != NULL)
        return executionEngine->getPointerToFunction(function);
    function = getFunction(name, types.size(), PointerType::getUnqual(TheMatrixStruct));

    auto kernelPointer = kernels.find(description);
    if (kernelPointer == kernels.end()) {
        kernels[description] = KernelBuilder(description);
        kernelPointer = kernels.find(description);
    }
    if (!(*kernelPointer).second.makeAllocation(function, types))
        return MatrixBuilder::cleanup(function);

    static FunctionPassManager *functionPassManager = NULL;
    if (functionPassManager == NULL) {
        functionPassManager = new FunctionPassManager(TheModule);
        functionPassManager->add(createVerifierPass(PrintMessageAction));
    }
    functionPassManager->run(*function);

    return executionEngine->getPointerToFunction(function);
}

void *likely_compile_kernel(likely_description description, likely_arity n, likely_const_mat src, ...)
{
    vector<likely_const_mat> srcs;
    va_list ap;
    va_start(ap, src);
    for (int i=0; i<n; i++) {
        srcs.push_back(src);
        src = va_arg(ap, likely_const_mat);
    }
    va_end(ap);
    return likely_compile_kernel_n(description, n, srcs.data());
}

void *likely_compile_kernel_n(likely_description description, likely_arity n, likely_const_mat *srcs)
{
    vector<likely_type> types;
    for (int i=0; i<n; i++) {
        likely_const_mat src = srcs[i];
        likely_assert(src && (src->type != likely_type_null), "likely_compile_kernel_n null matrix at index: %d", i);
        types.push_back(src->type);
    }

    lock_guard<recursive_mutex> lock(makerLock);
    const string name = mangledName(description, types)+"_kernel";

    Function *function = TheModule->getFunction(name);
    if (function != NULL)
        return executionEngine->getPointerToFunction(function);
    function = getFunction(name, types.size(), Type::getVoidTy(getGlobalContext()), PointerType::getUnqual(TheMatrixStruct), Type::getInt32Ty(getGlobalContext()), Type::getInt32Ty(getGlobalContext()));

    if (!kernels[description].makeKernel(function)) return MatrixBuilder::cleanup(function);

    static FunctionPassManager *functionPassManager = NULL;
    if (functionPassManager == NULL) {
        PassRegistry &registry = *PassRegistry::getPassRegistry();
        initializeScalarOpts(registry);

        functionPassManager = new FunctionPassManager(TheModule);
        functionPassManager->add(createVerifierPass(PrintMessageAction));
        targetMachine->addAnalysisPasses(*functionPassManager);
        functionPassManager->add(new TargetLibraryInfo(Triple(TheModule->getTargetTriple())));
        functionPassManager->add(new DataLayout(TheModule));
        functionPassManager->add(createBasicAliasAnalysisPass());
        functionPassManager->add(createLICMPass());
        functionPassManager->add(createLoopVectorizePass());
        functionPassManager->add(createInstructionCombiningPass());
        functionPassManager->add(createEarlyCSEPass());
        functionPassManager->add(createCFGSimplificationPass());
//        DebugFlag = true;
    }
    functionPassManager->run(*function);
//    function->dump();

    return executionEngine->getPointerToFunction(function);
}

void likely_parallel_dispatch(void *kernel, likely_arity arity, likely_size start, likely_size stop, likely_mat src, ...)
{
    workersActive.lock();

    workersRemaining = workers.size();
    workerKernel = kernel;
    workerArity = arity;
    workerStart = start;
    workerStop = stop;

    va_list ap;
    va_start(ap, src);
    for (int i=0; i<arity+1; i++) {
        workerMatricies[i] = src;
        src = va_arg(ap, likely_mat);
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
        {"get", lua_likely_get},
        {"set", lua_likely_set},
        {"elements", lua_likely_elements},
        {"bytes", lua_likely_bytes},
        {"retain", lua_likely_retain},
        {"release", lua_likely_release},
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
