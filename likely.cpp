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

static void checkLua(lua_State *L, int error = true)
{
    if (!error) return;

    fprintf(stderr, "Lua stack dump:\n");
    const int top = lua_gettop(L);
    for (int i = 1; i <= top; i++) {
        const int t = lua_type(L, i);
        switch (t) {
          case LUA_TSTRING:  fprintf(stderr, "\t'%s'\n", lua_tostring(L, i));                   break;
          case LUA_TBOOLEAN: fprintf(stderr, "\t%s\n", lua_toboolean(L, i) ? "true" : "false"); break;
          case LUA_TNUMBER:  fprintf(stderr, "\t%f\n", lua_tonumber(L, i));                     break;
          default:           fprintf(stderr, "\t%s\n", lua_typename(L, t));                     break;
        }
    }

    const string errorMessage = lua_tostring(L, -1); lua_pop(L, 1);
    likely_assert(false, "%s", errorMessage.c_str());
}

static likely_mat checkLuaMat(lua_State *L, int index = 1)
{
    return (likely_mat) luaL_checkudata(L, index, "likely");
}

static int lua_likely__tostring(lua_State *L)
{
    likely_assert(lua_gettop(L) == 1, "'__tostring' expected 1 argument, got: %d", lua_gettop(L));
    likely_const_mat m = checkLuaMat(L);
    lua_pushfstring(L, "Likely %dx%dx%dx%d %s %p", m->channels, m->columns, m->rows, m->frames, likely_hash_to_string(m->hash), m);
    return 1;
}

static inline int likely_get(likely_hash hash, likely_hash_field mask) { return hash & mask; }
static inline void likely_set(likely_hash *hash, int i, likely_hash_field mask) { *hash &= ~mask; *hash |= i & mask; }
static inline bool likely_get_bool(likely_hash hash, likely_hash_field mask) { return hash & mask; }
static inline void likely_set_bool(likely_hash *hash, bool b, likely_hash_field mask) { b ? *hash |= mask : *hash &= ~mask; }
int  likely_depth(likely_hash hash) { return likely_get(hash, likely_hash_depth); }
void likely_set_depth(likely_hash *hash, int depth) { likely_set(hash, depth, likely_hash_depth); }
bool likely_signed(likely_hash hash) { return likely_get_bool(hash, likely_hash_signed); }
void likely_set_signed(likely_hash *hash, bool signed_) { likely_set_bool(hash, signed_, likely_hash_signed); }
bool likely_floating(likely_hash hash) { return likely_get_bool(hash, likely_hash_floating); }
void likely_set_floating(likely_hash *hash, bool floating) { likely_set_bool(hash, floating, likely_hash_floating); }
int  likely_type(likely_hash hash) { return likely_get(hash, likely_hash_type); }
void likely_set_type(likely_hash *hash, int type) { likely_set(hash, type, likely_hash_type); }
bool likely_parallel(likely_hash hash) { return likely_get_bool(hash, likely_hash_parallel); }
void likely_set_parallel(likely_hash *hash, bool parallel) { likely_set_bool(hash, parallel, likely_hash_parallel); }
bool likely_heterogeneous(likely_hash hash) { return likely_get_bool(hash, likely_hash_heterogeneous); }
void likely_set_heterogeneous(likely_hash *hash, bool heterogeneous) { likely_set_bool(hash, heterogeneous, likely_hash_heterogeneous); }
bool likely_single_channel(likely_hash hash) { return likely_get_bool(hash, likely_hash_single_channel); }
void likely_set_single_channel(likely_hash *hash, bool single_channel) { likely_set_bool(hash, single_channel, likely_hash_single_channel); }
bool likely_single_column(likely_hash hash) { return likely_get_bool(hash, likely_hash_single_column); }
void likely_set_single_column(likely_hash *hash, bool single_column) { likely_set_bool(hash, single_column, likely_hash_single_column); }
bool likely_single_row(likely_hash hash) { return likely_get_bool(hash, likely_hash_single_row); }
void likely_set_single_row(likely_hash *hash, bool single_row) { likely_set_bool(hash, single_row, likely_hash_single_row); }
bool likely_single_frame(likely_hash hash) { return likely_get_bool(hash, likely_hash_single_frame); }
void likely_set_single_frame(likely_hash *hash, bool single_frame) { likely_set_bool(hash, single_frame, likely_hash_single_frame); }
bool likely_owner(likely_hash hash) { return likely_get_bool(hash, likely_hash_owner); }
void likely_set_owner(likely_hash *hash, bool owner) { likely_set_bool(hash, owner, likely_hash_owner); }
int  likely_reserved(likely_hash hash) { return likely_get(hash, likely_hash_reserved); }
void likely_set_reserved(likely_hash *hash, int reserved) { likely_set(hash, reserved, likely_hash_reserved); }

static int lua_likely_get(lua_State *L)
{
    likely_assert(lua_gettop(L) == 2, "'get' expected 2 arguments, got: %d", lua_gettop(L));
    likely_const_mat m = checkLuaMat(L);
    const char *field = lua_tostring(L, 2);
    if      (!strcmp(field, "likely"))        lua_pushstring(L, "matrix");
    else if (!strcmp(field, "data"))          lua_pushlightuserdata(L, m->data);
    else if (!strcmp(field, "hash"))          lua_pushstring(L, likely_hash_to_string(m->hash));
    else if (!strcmp(field, "channels"))      lua_pushinteger(L, m->channels);
    else if (!strcmp(field, "columns"))       lua_pushinteger(L, m->columns);
    else if (!strcmp(field, "rows"))          lua_pushinteger(L, m->rows);
    else if (!strcmp(field, "frames"))        lua_pushinteger(L, m->frames);
    else if (!strcmp(field, "depth"))         lua_pushinteger(L, likely_depth(m->hash));
    else if (!strcmp(field, "signed"))        lua_pushboolean(L, likely_signed(m->hash));
    else if (!strcmp(field, "floating"))      lua_pushboolean(L, likely_floating(m->hash));
    else if (!strcmp(field, "type"))          lua_pushinteger(L, likely_type(m->hash));
    else if (!strcmp(field, "parallel"))      lua_pushboolean(L, likely_parallel(m->hash));
    else if (!strcmp(field, "heterogeneous")) lua_pushboolean(L, likely_heterogeneous(m->hash));
    else if (!strcmp(field, "singleChannel")) lua_pushboolean(L, likely_single_channel(m->hash));
    else if (!strcmp(field, "singleColumn"))  lua_pushboolean(L, likely_single_column(m->hash));
    else if (!strcmp(field, "singleRow"))     lua_pushboolean(L, likely_single_row(m->hash));
    else if (!strcmp(field, "singleFrame"))   lua_pushboolean(L, likely_single_frame(m->hash));
    else if (!strcmp(field, "owner"))         lua_pushboolean(L, likely_owner(m->hash));
    else if (!strcmp(field, "reserved"))      lua_pushinteger(L, likely_reserved(m->hash));
    else                                    { likely_assert(false, "unrecognized field: %s", field); return 0; }
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
    else if (!strcmp(field, "hash"))          m->hash = likely_string_to_hash(lua_tostring(L, 3));
    else if (!strcmp(field, "channels"))    { m->channels = lua_tointegerx(L, 3, &isnum); likely_assert(isnum, "'set' expected channels to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "columns"))     { m->columns  = lua_tointegerx(L, 3, &isnum); likely_assert(isnum, "'set' expected columns to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "rows"))        { m->rows     = lua_tointegerx(L, 3, &isnum); likely_assert(isnum, "'set' expected rows to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "frames"))      { m->frames   = lua_tointegerx(L, 3, &isnum); likely_assert(isnum, "'set' expected frames to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "depth"))       { likely_set_depth(&m->hash, lua_tointegerx(L, 3, &isnum)); likely_assert(isnum, "'set' expected depth to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "signed"))        likely_set_signed(&m->hash, lua_toboolean(L, 3));
    else if (!strcmp(field, "floating"))      likely_set_floating(&m->hash, lua_toboolean(L, 3));
    else if (!strcmp(field, "type"))        { likely_set_type(&m->hash, lua_tointegerx(L, 3, &isnum)); likely_assert(isnum, "'set' expected type to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "parallel"))      likely_set_parallel(&m->hash, lua_toboolean(L, 3));
    else if (!strcmp(field, "heterogeneous")) likely_set_heterogeneous(&m->hash, lua_toboolean(L, 3));
    else if (!strcmp(field, "singleChannel")) likely_set_single_channel(&m->hash, lua_toboolean(L, 3));
    else if (!strcmp(field, "singleColumn"))  likely_set_single_column(&m->hash, lua_toboolean(L, 3));
    else if (!strcmp(field, "singleRow"))     likely_set_single_row(&m->hash, lua_toboolean(L, 3));
    else if (!strcmp(field, "singleFrame"))   likely_set_single_frame(&m->hash, lua_toboolean(L, 3));
    else if (!strcmp(field, "owner"))         likely_set_owner(&m->hash, lua_toboolean(L, 3));
    else if (!strcmp(field, "reserved"))    { likely_set_reserved(&m->hash, lua_tointegerx(L, 3, &isnum)); likely_assert(isnum, "'set' expected reserved to be an integer, got: %s", lua_tostring(L, 3)); }
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
    return uint64_t(likely_depth(m->hash)) * uint64_t(likely_elements(m)) / uint64_t(8);
}

static int lua_likely_bytes(lua_State *L)
{
    likely_assert(lua_gettop(L) == 1, "'bytes' expected 1 argument, got: %d", lua_gettop(L));
    lua_pushinteger(L, likely_bytes(checkLuaMat(L)));
    return 1;
}

likely_mat likely_new(likely_hash hash, likely_size channels, likely_size columns, likely_size rows, likely_size frames, likely_data *data, bool clone)
{
    likely_mat m = new likely_matrix();
    likely_initialize(m, hash, channels, columns, rows, frames, data, clone);
    return m;
}

static likely_mat newLuaMat(lua_State *L)
{
    likely_mat m = (likely_mat) lua_newuserdata(L, sizeof(likely_matrix));
    luaL_getmetatable(L, "likely");
    lua_setmetatable(L, -2);
    return m;
}

static int lua_likely_new(lua_State *L)
{
    likely_hash hash = likely_hash_null;
    likely_size channels = 0;
    likely_size columns = 0;
    likely_size rows = 0;
    likely_size frames = 0;
    likely_data *data = NULL;
    bool clone = true;

    int isnum;
    const int argc = lua_gettop(L);
    switch (argc) {
      case 7: clone    = lua_toboolean(L, 7);
      case 6: data     = (likely_data*) lua_touserdata(L, 6);
      case 5: frames   = lua_tointegerx(L, 5, &isnum); likely_assert(isnum, "'new' expected frames to be an integer, got: %s", lua_tostring(L, 5));
      case 4: rows     = lua_tointegerx(L, 4, &isnum); likely_assert(isnum, "'new' expected rows to be an integer, got: %s", lua_tostring(L, 4));
      case 3: columns  = lua_tointegerx(L, 3, &isnum); likely_assert(isnum, "'new' expected columns to be an integer, got: %s", lua_tostring(L, 3));
      case 2: channels = lua_tointegerx(L, 2, &isnum); likely_assert(isnum, "'new' expected channels to be an integer, got: %s", lua_tostring(L, 2));
      case 1: hash     = likely_string_to_hash(lua_tostring(L, 1));
      case 0: break;
      default: likely_assert(false, "'new' expected no more than 7 arguments, got: %d", argc);
    }

    likely_initialize(newLuaMat(L), hash, channels, columns, rows, frames, data, clone);
    return 1;
}

void likely_initialize(likely_mat m, likely_hash hash, likely_size channels, likely_size columns, likely_size rows, likely_size frames, likely_data *data, bool clone)
{
    m->hash = hash;
    m->channels = channels;
    m->columns = columns;
    m->rows = rows;
    m->frames = frames;

    if (!data || clone) {
        likely_allocate(m);
        if (data && clone)
            memcpy(m->data, data, likely_bytes(m));
    } else {
        m->data = data;
    }
}

static int lua_likely_initialize(lua_State *L)
{
    likely_mat  m = NULL;
    likely_hash hash = likely_hash_null;
    likely_size channels = 0;
    likely_size columns = 0;
    likely_size rows = 0;
    likely_size frames = 0;
    likely_data *data = NULL;
    bool clone = true;

    int isnum;
    const int argc = lua_gettop(L);
    switch (argc) {
      case 8: clone    = lua_toboolean(L, 8);
      case 7: data     = (likely_data*) lua_touserdata(L, 7);
      case 6: frames   = lua_tonumberx(L, 6, &isnum); likely_assert(isnum, "'new' expected frames to be an integer, got: %s", lua_tostring(L, 6));
      case 5: rows     = lua_tonumberx(L, 5, &isnum); likely_assert(isnum, "'new' expected rows to be an integer, got: %s", lua_tostring(L, 5));
      case 4: columns  = lua_tonumberx(L, 4, &isnum); likely_assert(isnum, "'new' expected columns to be an integer, got: %s", lua_tostring(L, 4));
      case 3: channels = lua_tonumberx(L, 3, &isnum); likely_assert(isnum, "'new' expected channels to be an integer, got: %s", lua_tostring(L, 3));
      case 2: hash     = likely_string_to_hash(lua_tostring(L, 2));
      case 1: m        = checkLuaMat(L); break;
      default: likely_assert(false, "'initialize' expected 1-8 arguments, got: %d", argc);
    }

    likely_initialize(m, hash, channels, columns, rows, frames, data, clone);
    return 0;
}

likely_mat likely_clone(likely_const_mat m)
{
    return likely_new(m->hash, m->channels, m->columns, m->rows, m->frames, m->data);
}

static int lua_likely_clone(lua_State *L)
{
    likely_assert(lua_gettop(L) == 1, "'clone' expected 1 argument, got: %d", lua_gettop(L));
    likely_const_mat m = checkLuaMat(L);
    likely_initialize(newLuaMat(L), m->hash, m->channels, m->columns, m->rows, m->frames, m->data);
    return 1;
}

void likely_delete(likely_mat m)
{
    likely_free(m);
    delete m;
}

static int lua_likely_delete(lua_State *L)
{
    likely_assert(lua_gettop(L) == 1, "'delete' expected 1 argument, got: %d", lua_gettop(L));
    likely_mat m = checkLuaMat(L);
    likely_free(m);
    return 0;
}

void likely_allocate(likely_mat m)
{
    const likely_size bytes = likely_bytes(m);
    if (bytes == 0) {
        m->data = NULL;
        return;
    }
    size_t alignment = MaxRegisterWidth;
    uintptr_t r = (uintptr_t)malloc(bytes + --alignment + 2);
    uintptr_t o = (r + 2 + alignment) & ~(uintptr_t)alignment;
    ((uint16_t*)o)[-1] = (uint16_t)(o-r);
    m->data = (likely_data*)o;
    likely_set_owner(&m->hash, true);
}

static int lua_likely_allocate(lua_State *L)
{
    likely_assert(lua_gettop(L) == 1, "'allocate' expected 1 argument, got: %d", lua_gettop(L));
    likely_allocate(checkLuaMat(L));
    return 0;
}

void likely_free(likely_mat m)
{
    if (!m || !likely_owner(m->hash) || !m->data) return;
    free((void*)((uintptr_t)m->data-((uint16_t*)m->data)[-1]));
    m->data = NULL;
    likely_set_owner(&m->hash, false);
}

static int lua_likely_free(lua_State *L)
{
    likely_assert(lua_gettop(L) == 1, "'free' expected 1 argument, got: %d", lua_gettop(L));
    likely_free(checkLuaMat(L));
    return 0;
}

static likely_mat likelyReadHelper(const char *file_name, likely_mat image, lua_State *L = NULL)
{
    cv::Mat m = cv::imread(file_name, CV_LOAD_IMAGE_UNCHANGED);
    lua_likely_assert(L, m.data, "'read' failed to open: %s", file_name);
    if (m.data) {
        return fromCvMat(m, true, image);
    } else {
        if (image) {
            likely_initialize(image);
            return image;
        }
        return NULL;
    }
}

likely_mat likely_read(const char *file_name, likely_mat image)
{
    return likelyReadHelper(file_name, image);
}

static int lua_likely_read(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 1, "'read' expected 1 argument, got: %d", lua_gettop(L));
    likelyReadHelper(lua_tostring(L, 1), newLuaMat(L), L);
    return 1;
}

void likely_write(likely_const_mat image, const char *file_name)
{
    cv::imwrite(file_name, toCvMat(image));
}

static int lua_likely_write(lua_State *L)
{
    likely_assert(lua_gettop(L) == 2, "'write' expected 2 arguments, got: %d", lua_gettop(L));
    likely_write(checkLuaMat(L), lua_tostring(L, 2));
    return 0;
}

likely_mat likely_decode(likely_const_mat buffer, likely_mat image)
{
    return fromCvMat(cv::imdecode(toCvMat(buffer), CV_LOAD_IMAGE_UNCHANGED), true, image);
}

static int lua_likely_decode(lua_State *L)
{
    likely_assert(lua_gettop(L) == 1, "'decode' expected 1 argument, got: %d", lua_gettop(L));
    likely_decode(checkLuaMat(L), newLuaMat(L));
    return 1;
}

likely_mat likely_encode(likely_const_mat image, const char *extension, likely_mat buffer)
{
    vector<uchar> buf;
    cv::imencode(extension, toCvMat(image), buf);
    return fromCvMat(cv::Mat(buf), true, buffer);
}

static int lua_likely_encode(lua_State *L)
{
    likely_assert(lua_gettop(L) == 2, "'write' expected 2 arguments, got: %d", lua_gettop(L));
    likely_encode(checkLuaMat(L), lua_tostring(L, 2), newLuaMat(L));
    return 1;
}

double likely_element(likely_const_mat m, likely_size c, likely_size x, likely_size y, likely_size t)
{
    likely_assert(m != NULL, "likely_element received a null matrix");
    const int columnStep = m->channels;
    const int rowStep = m->columns * columnStep;
    const int frameStep = m->rows * rowStep;
    const int index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (likely_type(m->hash)) {
      case likely_hash_u8:  return  ((uint8_t*)m->data)[index];
      case likely_hash_u16: return ((uint16_t*)m->data)[index];
      case likely_hash_u32: return ((uint32_t*)m->data)[index];
      case likely_hash_u64: return ((uint64_t*)m->data)[index];
      case likely_hash_i8:  return   ((int8_t*)m->data)[index];
      case likely_hash_i16: return  ((int16_t*)m->data)[index];
      case likely_hash_i32: return  ((int32_t*)m->data)[index];
      case likely_hash_i64: return  ((int64_t*)m->data)[index];
      case likely_hash_f32: return    ((float*)m->data)[index];
      case likely_hash_f64: return   ((double*)m->data)[index];
      default: likely_assert(false, "likely_element unsupported type");
    }
    return numeric_limits<double>::quiet_NaN();
}

void likely_set_element(likely_mat m, double value, likely_size c, likely_size x, likely_size y, likely_size t)
{
    likely_assert(m != NULL, "likely_set_element received a null matrix");
    const int columnStep = m->channels;
    const int rowStep = m->channels * columnStep;
    const int frameStep = m->rows * rowStep;
    const int index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (likely_type(m->hash)) {
      case likely_hash_u8:   ((uint8_t*)m->data)[index] = value; break;
      case likely_hash_u16: ((uint16_t*)m->data)[index] = value; break;
      case likely_hash_u32: ((uint32_t*)m->data)[index] = value; break;
      case likely_hash_u64: ((uint64_t*)m->data)[index] = value; break;
      case likely_hash_i8:    ((int8_t*)m->data)[index] = value; break;
      case likely_hash_i16:  ((int16_t*)m->data)[index] = value; break;
      case likely_hash_i32:  ((int32_t*)m->data)[index] = value; break;
      case likely_hash_i64:  ((int64_t*)m->data)[index] = value; break;
      case likely_hash_f32:    ((float*)m->data)[index] = value; break;
      case likely_hash_f64:   ((double*)m->data)[index] = value; break;
      default: likely_assert(false, "likely_set_element unsupported type");
    }
}

const char *likely_hash_to_string(likely_hash h)
{
    static string hashString; // Provides return value persistence

    stringstream hashStream;
    hashStream << (likely_floating(h) ? "f" : (likely_signed(h) ? "i" : "u"));
    hashStream << likely_depth(h);

    if (likely_parallel(h))       hashStream << "P";
    if (likely_heterogeneous(h))  hashStream << "H";
    if (likely_single_channel(h)) hashStream << "C";
    if (likely_single_column(h))  hashStream << "X";
    if (likely_single_row(h))     hashStream << "Y";
    if (likely_single_frame(h))   hashStream << "T";
    if (likely_owner(h))          hashStream << "O";

    hashString = hashStream.str();
    return hashString.c_str();
}

likely_hash likely_string_to_hash(const char *str)
{
    likely_hash h = likely_hash_null;
    const size_t len = strlen(str);
    if ((str == NULL) || (len == 0))
        return h;

    if (str[0] == 'f') likely_set_floating(&h, true);
    if (str[0] != 'u') likely_set_signed(&h, true);
    likely_set_depth(&h, atoi(str+1));

    size_t startIndex = 1;
    while ((str[startIndex] >= '0') && (str[startIndex] <= '9'))
        startIndex++;

    for (size_t i=startIndex; i<len; i++) {
        if (str[i] == 'P') likely_set_parallel(&h, true);
        if (str[i] == 'H') likely_set_heterogeneous(&h, true);
        if (str[i] == 'C') likely_set_single_channel(&h, true);
        if (str[i] == 'X') likely_set_single_column(&h, true);
        if (str[i] == 'Y') likely_set_single_row(&h, true);
        if (str[i] == 'T') likely_set_single_frame(&h, true);
        if (str[i] == 'O') likely_set_owner(&h, true);
    }

    return h;
}

void likely_print(likely_const_mat m)
{
    if ((m == NULL) || (m->data == NULL)) return;
    const int type = likely_type(m->hash);
    for (uint t=0; t<m->frames; t++) {
        for (uint y=0; y<m->rows; y++) {
            cout << (m->rows > 1 ? (y == 0 ? "[" : " ") : "");
            for (uint x=0; x<m->columns; x++) {
                for (uint c=0; c<m->channels; c++) {
                    const double value = likely_element(m, c, x, y, t);
                    switch (type) {
                      case likely_hash_u8:  cout <<  (uint8_t)value; break;
                      case likely_hash_u16: cout << (uint16_t)value; break;
                      case likely_hash_u32: cout << (uint32_t)value; break;
                      case likely_hash_u64: cout << (uint64_t)value; break;
                      case likely_hash_i8:  cout <<   (int8_t)value; break;
                      case likely_hash_i16: cout <<  (int16_t)value; break;
                      case likely_hash_i32: cout <<  (int32_t)value; break;
                      case likely_hash_i64: cout <<  (int64_t)value; break;
                      case likely_hash_f32: cout <<    (float)value; break;
                      case likely_hash_f64: cout <<   (double)value; break;
                      default: likely_assert(false, "likely_print_matrix unsupported type.");
                    }
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
    checkLua(L, luaL_dostring(L, likely_standard_library()));
    return L;
}

namespace likely
{

struct MatrixBuilder
{
    IRBuilder<> *b;
    Function *f;
    Twine n;
    likely_hash h;
    Value *v;

    struct Loop {
        BasicBlock *body, *exit;
        PHINode *i;
        Value *stop;
        MDNode *node;
    };
    stack<Loop> loops;

    MatrixBuilder() : b(NULL), f(NULL), h(likely_hash_null), v(NULL) {}
    MatrixBuilder(IRBuilder<> *builder, Function *function, const Twine &name, likely_hash hash = likely_hash_null, Value *value = NULL)
        : b(builder), f(function), n(name), h(hash), v(value) {}
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
    Constant *autoConstant(double value) const { return likely_floating(h) ? ((likely_depth(h) == 64) ? constant(value) : constant(float(value))) : constant(int(value), likely_depth(h)); }
    AllocaInst *autoAlloca(double value) const { AllocaInst *alloca = b->CreateAlloca(ty(), 0, n); b->CreateStore(autoConstant(value), alloca); return alloca; }

    Value *data(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 0), n+"_data"); }
    Value *data(Value *matrix, Type *type) const { return b->CreatePointerCast(data(matrix), type); }
    Value *hash(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 1), n+"_hash"); }
    Value *channels(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 2), n+"_channels"); }
    Value *columns(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 3), n+"_columns"); }
    Value *rows(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 4), n+"_rows"); }
    Value *frames(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 5), n+"_frames"); }

    Value *data(bool cast = true) const { return cast ? data(v, ty(true)) : data(v); }
    Value *hash() const { return hash(v); }
    Value *channels() const { return likely_single_channel(h) ? static_cast<Value*>(one()) : channels(v); }
    Value *columns() const { return likely_single_column(h) ? static_cast<Value*>(one()) : columns(v); }
    Value *rows() const { return likely_single_row(h) ? static_cast<Value*>(one()) : rows(v); }
    Value *frames() const { return likely_single_frame(h) ? static_cast<Value*>(one()) : frames(v); }

    void setData(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 0)); }
    void setHash(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 1)); }
    void setChannels(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 2)); }
    void setColumns(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 3)); }
    void setRows(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 4)); }
    void setFrames(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 5)); }

    void setData(Value *value) const { setData(v, value); }
    void setHash(Value *value) const { setHash(v, value); }
    void setChannels(Value *value) const { setChannels(v, value); }
    void setColumns(Value *value) const { setColumns(v, value); }
    void setRows(Value *value) const { setRows(v, value); }
    void setFrames(Value *value) const { setFrames(v, value); }

    void copyHeaderTo(Value *matrix) const {
        setHash(matrix, hash());
        setChannels(matrix, channels());
        setColumns(matrix, columns());
        setRows(matrix, rows());
        setFrames(matrix, frames());
    }

    void deallocate() const {
        static Function *free = TheModule->getFunction("free");
        if (!free) {
            Type *freeReturn = Type::getVoidTy(getGlobalContext());
            vector<Type*> freeParams;
            freeParams.push_back(Type::getInt8PtrTy(getGlobalContext()));
            FunctionType* freeType = FunctionType::get(freeReturn, freeParams, false);
            free = Function::Create(freeType, GlobalValue::ExternalLinkage, "free", TheModule);
            free->setCallingConv(CallingConv::C);
        }

        vector<Value*> freeArgs;
        freeArgs.push_back(b->CreateStructGEP(v, 0));
        b->CreateCall(free, freeArgs);
        setData(ConstantPointerNull::get(Type::getInt8PtrTy(getGlobalContext())));
    }

    Value *get(int mask) const { return b->CreateAnd(hash(), constant(mask, 8*sizeof(likely_hash))); }
    void set(int value, int mask) const { setHash(b->CreateOr(b->CreateAnd(hash(), constant(~mask, 8*sizeof(likely_hash))), b->CreateAnd(constant(value, 8*sizeof(likely_hash)), constant(mask, 8*sizeof(likely_hash))))); }
    void setBit(bool on, int mask) const { on ? setHash(b->CreateOr(hash(), constant(mask, 8*sizeof(likely_hash)))) : setHash(b->CreateAnd(hash(), constant(~mask, 8*sizeof(likely_hash)))); }

    Value *depth() const { return get(likely_hash_depth); }
    void setDepth(int depth) const { set(depth, likely_hash_depth); }
    Value *isSigned() const { return get(likely_hash_signed); }
    void setSigned(bool isSigned) const { setBit(isSigned, likely_hash_signed); }
    Value *isFloating() const { return get(likely_hash_floating); }
    void setFloating(bool isFloating) const { if (isFloating) setSigned(true); setBit(isFloating, likely_hash_floating); }
    Value *type() const { return get(likely_hash_depth + likely_hash_floating + likely_hash_signed); }
    void setType(int type) const { set(type, likely_hash_depth + likely_hash_floating + likely_hash_signed); }
    Value *isParallel() const { return get(likely_hash_parallel); }
    void setParallel(bool isParallel) const { setBit(isParallel, likely_hash_parallel); }
    Value *isHeterogeneous() const { return get(likely_hash_heterogeneous); }
    void setHeterogeneous(bool isHeterogeneous) const { setBit(isHeterogeneous, likely_hash_heterogeneous); }
    Value *isSingleChannel() const { return get(likely_hash_single_channel); }
    void setSingleChannel(bool isSingleChannel) const { setBit(isSingleChannel, likely_hash_single_channel); }
    Value *isSingleColumn() const { return get(likely_hash_single_column); }
    void setSingleColumn(bool isSingleColumn) { setBit(isSingleColumn, likely_hash_single_column); }
    Value *isSingleRow() const { return get(likely_hash_single_row); }
    void setSingleRow(bool isSingleRow) const { setBit(isSingleRow, likely_hash_single_row); }
    Value *isSingleFrame() const { return get(likely_hash_single_frame); }
    void setSingleFrame(bool isSingleFrame) const { setBit(isSingleFrame, likely_hash_single_frame); }
    Value *isOwner() const { return get(likely_hash_owner); }
    void setOwner(bool isOwner) const { setBit(isOwner, likely_hash_owner); }
    Value *reserved() const { return get(likely_hash_reserved); }
    void setReserved(int reserved) const { set(reserved, likely_hash_reserved); }

    Value *elements() const { return b->CreateMul(b->CreateMul(b->CreateMul(channels(), columns()), rows()), frames()); }
    Value *bytes() const { return b->CreateMul(b->CreateUDiv(b->CreateCast(Instruction::ZExt, depth(), Type::getInt32Ty(getGlobalContext())), constant(8, 32)), elements()); }

    Value *columnStep() const { Value *columnStep = channels(); columnStep->setName(n+"_cStep"); return columnStep; }
    Value *rowStep() const { return b->CreateMul(columns(), columnStep(), n+"_rStep"); }
    Value *frameStep() const { return b->CreateMul(rows(), rowStep(), n+"_tStep"); }

    Value *index(Value *c) const { return likely_single_channel(h) ? constant(0) : c; }
    Value *index(Value *c, Value *x) const { return likely_single_column(h) ? index(c) : b->CreateAdd(b->CreateMul(x, columnStep()), index(c)); }
    Value *index(Value *c, Value *x, Value *y) const { return likely_single_row(h) ? index(c, x) : b->CreateAdd(b->CreateMul(y, rowStep()), index(c, x)); }
    Value *index(Value *c, Value *x, Value *y, Value *f) const { return likely_single_frame(h) ? index(c, x, y) : b->CreateAdd(b->CreateMul(f, frameStep()), index(c, x, y)); }

    void deindex(Value *i, Value **c) const {
        *c = likely_single_channel(h) ? constant(0) : i;
    }
    void deindex(Value *i, Value **c, Value **x) const {
        Value *rem;
        if (likely_single_column(h)) {
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
        if (likely_single_row(h)) {
            rem = i;
            *y = constant(0);
        } else {
            Value *step = rowStep();
            rem = b->CreateURem(i, step, n+"_yRem");
            *y = b->CreateExactUDiv(b->CreateSub(i, rem), step, n+"_y");
        }
        deindex(rem, c, x);
    }
    void deindex(Value *i, Value **c, Value **x, Value **y, Value **t) const {
        Value *rem;
        if (likely_single_frame(h)) {
            rem = i;
            *t = constant(0);
        } else {
            Value *step = frameStep();
            rem = b->CreateURem(i, step, n+"_tRem");
            *t = b->CreateExactUDiv(b->CreateSub(i, rem), step, n+"_t");
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

    Value *cast(Value *i, likely_hash hash) const { return (likely_type(h) == likely_type(hash)) ? i : b->CreateCast(CastInst::getCastOpcode(i, likely_signed(h), Type::getFloatTy(getGlobalContext()), likely_signed(h)), i, Type::getFloatTy(getGlobalContext())); }
    Value *add(Value *i, Value *j) const { return likely_floating(h) ? b->CreateFAdd(i, j, n) : b->CreateAdd(i, j, n); }
    Value *subtract(Value *i, Value *j) const { return likely_floating(h) ? b->CreateFSub(i, j, n) : b->CreateSub(i, j, n); }
    Value *multiply(Value *i, Value *j) const { return likely_floating(h) ? b->CreateFMul(i, j, n) : b->CreateMul(i, j, n); }
    Value *divide(Value *i, Value *j) const { return likely_floating(h) ? b->CreateFDiv(i, j, n) : (likely_signed(h) ? b->CreateSDiv(i,j, n) : b->CreateUDiv(i, j, n)); }

    Value *intrinsic(Value *i, Intrinsic::ID id) const { vector<Type*> args; args.push_back(i->getType()); Function *intrinsic = Intrinsic::getDeclaration(TheModule, id, args); return b->CreateCall(intrinsic, i, n); }
    Value *log(Value *i) const { return intrinsic(i, Intrinsic::log); }
    Value *log2(Value *i) const { return intrinsic(i, Intrinsic::log2); }
    Value *log10(Value *i) const { return intrinsic(i, Intrinsic::log10); }
    Value *sin(Value *i) const { return intrinsic(i, Intrinsic::sin); }
    Value *cos(Value *i) const { return intrinsic(i, Intrinsic::cos); }
    Value *fabs(Value *i) const { return intrinsic(i, Intrinsic::fabs); }
    Value *sqrt(Value *i) const { return intrinsic(i, Intrinsic::sqrt); }
    Value *exp(Value *i) const { return intrinsic(i, Intrinsic::exp); }

    Value *compareLT(Value *i, Value *j) const { return likely_floating(h) ? b->CreateFCmpOLT(i, j) : (likely_signed(h) ? b->CreateICmpSLT(i, j) : b->CreateICmpULT(i, j)); }
    Value *compareGT(Value *i, Value *j) const { return likely_floating(h) ? b->CreateFCmpOGT(i, j) : (likely_signed(h) ? b->CreateICmpSGT(i, j) : b->CreateICmpUGT(i, j)); }

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

    static Type *ty(likely_hash hash, bool pointer = false)
    {
        const int bits = likely_depth(hash);
        const bool floating = likely_floating(hash);
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
    inline Type *ty(bool pointer = false) const { return ty(h, pointer); }
    inline vector<Type*> tys(bool pointer = false) const { return toVector<Type*>(ty(pointer)); }

    static void *cleanup(Function *f) { f->removeFromParent(); delete f; return NULL; }
    void *cleanup() { return cleanup(f); }
};

class KernelBuilder
{
    likely_description description;
    vector<string> stack;
    vector<likely_hash> hashes;
    MatrixBuilder kernel;
    PHINode *i = NULL;

public:
    KernelBuilder() = default;
    KernelBuilder(likely_description description_)
        : description(description_)
    {
        const char *isStack = "likely";
        if (!strncmp(description, isStack, strlen(isStack))) {
            // It's already in intermediate representation
            istringstream iss(description);
            copy(istream_iterator<string>(iss),
                 istream_iterator<string>(),
                 back_inserter< vector<string> >(stack));
            stack.erase(stack.begin()); // remove "likely"
        } else {
            // It needs to be interpreted
            lua_State *L = getLuaState();
            checkLua(L, luaL_dostring(L, (string("return ") + description).c_str()));
            const int top = lua_gettop(L);
            for (int j=1; j<=top; j++)
                stack.push_back(lua_tostring(L, j));
            lua_settop(L, 0);
        }
    }

    bool makeAllocation(Function *function, const vector<likely_hash> &hashes_)
    {
        hashes = hashes_;
        vector<Value*> srcs;
        Value *dst;
        getValues(function, srcs, dst);

        BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", function);
        IRBuilder<> builder(entry);

        likely_hash kernelHash = likely_hash_null;
        for (likely_hash hash : hashes)
            kernelHash |= hash;

        kernel = MatrixBuilder(&builder, function, "kernel", kernelHash, srcs[0]);
        kernel.copyHeaderTo(dst);
        builder.CreateRet(kernel.elements());
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

        if (likely_parallel(kernel.h)) {
            vector<likely_mat> mats;
            for (likely_hash hash : hashes) {
                likely_mat m = likely_new(hash);
                likely_set_parallel(&m->hash, false);
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
                likely_delete(m);
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

    static void getValues(Function *function, vector<Value*> &srcs, Value *&dst)
    {
        Function::arg_iterator args = function->arg_begin();
        int i = 0;
        while (args != function->arg_end()) {
            Value *src = args++;
            stringstream name; name << "src" << char(int('A')+(i++));
            src->setName(name.str());
            srcs.push_back(src);
        }
        dst = srcs.back();
        srcs.pop_back();
        dst->setName("dst");
    }

    static void getValues(Function *function, vector<Value*> &srcs, Value *&dst, Value *&start, Value *&stop)
    {
        getValues(function, srcs, dst);
        stop = dst;
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

static string mangledName(const string &description, const vector<likely_hash> &hashes)
{
    stringstream stream; stream << description;
    for (likely_hash hash : hashes)
        stream << "_" << likely_hash_to_string(hash);
    return stream.str();
}

static Function *getFunction(const string &description, likely_arity arity, Type *ret, Type *start = NULL, Type *stop = NULL)
{
    PointerType *matrixPointer = PointerType::getUnqual(TheMatrixStruct);
    Function *function;
    switch (arity) {
      case 0: function = cast<Function>(TheModule->getOrInsertFunction(description, ret, matrixPointer, start, stop, NULL)); break;
      case 1: function = cast<Function>(TheModule->getOrInsertFunction(description, ret, matrixPointer, matrixPointer,  start, stop, NULL)); break;
      case 2: function = cast<Function>(TheModule->getOrInsertFunction(description, ret, matrixPointer, matrixPointer, matrixPointer,  start, stop, NULL)); break;
      case 3: function = cast<Function>(TheModule->getOrInsertFunction(description, ret, matrixPointer, matrixPointer, matrixPointer, matrixPointer,  start, stop, NULL)); break;
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
                                             Type::getInt32Ty(getGlobalContext()),   // hash
                                             Type::getInt32Ty(getGlobalContext()),   // channels
                                             Type::getInt32Ty(getGlobalContext()),   // columns
                                             Type::getInt32Ty(getGlobalContext()),   // rows
                                             Type::getInt32Ty(getGlobalContext()),   // frames
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
    function = getFunction(description, n, Type::getVoidTy(getGlobalContext()));

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
        vector<Type*> allocationParams;
        for (int i=0; i<n+1; i++)
            allocationParams.push_back(PointerType::getUnqual(TheMatrixStruct));
        Type *allocationReturn = Type::getInt32Ty(getGlobalContext());
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
        vector<Type*> kernelParams;
        for (int i=0; i<n+1; i++)
            kernelParams.push_back(PointerType::getUnqual(TheMatrixStruct));
        kernelParams.push_back(Type::getInt32Ty(getGlobalContext()));
        kernelParams.push_back(Type::getInt32Ty(getGlobalContext()));
        Type *kernelReturn = Type::getVoidTy(getGlobalContext());
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
    Value *dst;
    KernelBuilder::getValues(function, srcs, dst);

    BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", function);
    IRBuilder<> builder(entry);

    vector<GlobalVariable*> kernelHashes;
    for (int i=0; i<n; i++) {
        GlobalVariable *kernelHash = cast<GlobalVariable>(TheModule->getOrInsertGlobal(string(description)+"_hash"+to_string(i), Type::getInt32Ty(getGlobalContext())));
        kernelHash->setInitializer(MatrixBuilder::constant(srcList[i]->hash, 8*sizeof(likely_hash)));
        kernelHashes.push_back(kernelHash);
    }

    vector<Value*> srcHashes;
    for (int i=0; i<n; i++)
        srcHashes.push_back(builder.CreateLoad(builder.CreateStructGEP(srcs[i], 1), "src_hash"+to_string(i)));

    Value *hashTest = MatrixBuilder::constant(true);
    for (int i=0; i<n; i++)
        hashTest = builder.CreateAnd(hashTest, builder.CreateICmpEQ(builder.CreateLoad(kernelHashes[i]), srcHashes[i]));

    BasicBlock *hashFail = BasicBlock::Create(getGlobalContext(), "hash_fail", function);
    BasicBlock *execute = BasicBlock::Create(getGlobalContext(), "execute", function);
    builder.CreateCondBr(hashTest, execute, hashFail);

    builder.SetInsertPoint(hashFail);
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
            builder.CreateStore(srcHashes[i], kernelHashes[i]);
        builder.CreateBr(execute);
    }

    builder.SetInsertPoint(execute);
    {
        vector<Value*> args(srcs); args.push_back(dst);
        Value *kernelSize = builder.CreateCall(builder.CreateLoad(allocationFunction), args);

        static Function *likely_allocate = NULL;
        if (likely_allocate == NULL) {
            Type *allocateReturn = Type::getVoidTy(getGlobalContext());
            vector<Type*> allocateParameters;
            allocateParameters.push_back(PointerType::getUnqual(TheMatrixStruct));
            FunctionType* allocateType = FunctionType::get(allocateReturn, allocateParameters, false);
            likely_allocate = Function::Create(allocateType, GlobalValue::ExternalLinkage, "likely_allocate", TheModule);
            likely_allocate->setCallingConv(CallingConv::C);
        }
        builder.CreateCall(likely_allocate, dst);

        BasicBlock *kernel = BasicBlock::Create(getGlobalContext(), "kernel", function);
        BasicBlock *exit = BasicBlock::Create(getGlobalContext(), "exit", function);
        builder.CreateCondBr(builder.CreateICmpUGT(kernelSize, MatrixBuilder::zero()), kernel, exit);

        builder.SetInsertPoint(kernel);
        args.push_back(MatrixBuilder::zero());
        args.push_back(kernelSize);
        builder.CreateCall(builder.CreateLoad(kernelFunction), args);
        builder.CreateBr(exit);

        builder.SetInsertPoint(exit);
        builder.CreateRetVoid();
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
    const int args = lua_gettop(L);
    likely_assert(args >= 1, "'compile' expected at least one argument");

    // Make sure we were given a function
    lua_getfield(L, 1, "likely");
    likely_assert(!strcmp("function", lua_tostring(L, -1)), "'compile' expected the function argument to be a function");
    lua_pop(L, 1);

    // Get the function's arity
    lua_getfield(L, 1, "arity");
    int isnum;
    likely_arity arity = lua_tonumberx(L, -1, &isnum);
    lua_pop(L, 1);
    likely_assert(isnum, "'compile' expected function to have a numerical arity");
    likely_assert(arity >= args -1, "'compile' was given a function with arity: %d but: %d default arguments", arity, args-1);

    // Get the default matricies
    vector<likely_const_mat> mats;
    for (int i=2; i<=args; i++)
        mats.push_back(checkLuaMat(L, i));
    while (mats.size() < arity)
        mats.push_back(NULL);

    // Get the function source code
    lua_pushvalue(L, 1);
    for (int i=0; i<arity; i++)
        lua_pushstring(L, "src"); // TODO: support arity > 1
    lua_call(L, arity, 1);
    string source = lua_tostring(L, -1);
    lua_pop(L, 1);

    // Compile the source code
    source.insert(0, "likely ");
    void *compiled = likely_compile_n(source.c_str(), mats.size(), mats.data());

    // Return the compiled function
    lua_newtable(L);
    lua_pushstring(L, "likely");
    lua_pushstring(L, "function");
    lua_settable(L, -3);
    lua_pushstring(L, "source");
    lua_pushlightuserdata(L, compiled);
    lua_settable(L, -3);
    lua_pushstring(L, "arity");
    lua_pushnumber(L, mats.size());
    lua_settable(L, -3);
    luaL_getmetatable(L, "likely_function");
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_likely__call(lua_State *L)
{
    const int args = lua_gettop(L);
    likely_assert(args >= 1, "'__call' expected at least one argument, got: %d", lua_gettop(L));
    lua_getfield(L, 1, "source");
    void *function = lua_touserdata(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, 1, "arity");
    const int n = lua_tonumber(L, -1);
    lua_pop(L, 1);
    likely_assert((args-1 == n) || (args-1 == n + 1), "'__call' expected: %d-%d arguments, got: %d", n, n+1, args-1);

    vector<likely_const_mat> srcs;
    for (int i=0; i<n; i++)
        srcs.push_back(checkLuaMat(L, i+2));

    likely_mat dst;
    if (args-1 == n) {
        dst = newLuaMat(L);
    } else {
        dst = checkLuaMat(L, args);
        lua_pushvalue(L, -1);
    }

    switch (n) {
      case 0: reinterpret_cast<likely_function_0>(function)(dst); break;
      case 1: reinterpret_cast<likely_function_1>(function)(srcs[0], dst); break;
      case 2: reinterpret_cast<likely_function_2>(function)(srcs[0], srcs[1], dst); break;
      case 3: reinterpret_cast<likely_function_3>(function)(srcs[0], srcs[1], srcs[2], dst); break;
      default: likely_assert(false, "__call invalid arity: %d", n);
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
    vector<likely_hash> hashes;
    for (int i=0; i<n; i++) {
        likely_const_mat src = srcs[i];
        likely_assert(src && (src->hash != likely_hash_null), "likely_compile_allocation_n null matrix at index: %d", i);
        hashes.push_back(src->hash);
    }

    lock_guard<recursive_mutex> lock(makerLock);
    const string name = mangledName(description, hashes)+"_allocation";

    Function *function = TheModule->getFunction(name);
    if (function != NULL)
        return executionEngine->getPointerToFunction(function);
    function = getFunction(name, hashes.size(), Type::getInt32Ty(getGlobalContext()));

    auto kernelPointer = kernels.find(description);
    if (kernelPointer == kernels.end()) {
        kernels[description] = KernelBuilder(description);
        kernelPointer = kernels.find(description);
    }
    if (!(*kernelPointer).second.makeAllocation(function, hashes)) return MatrixBuilder::cleanup(function);

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
    vector<likely_hash> hashes;
    for (int i=0; i<n; i++) {
        likely_const_mat src = srcs[i];
        likely_assert(src && (src->hash != likely_hash_null), "likely_compile_kernel_n null matrix at index: %d", i);
        hashes.push_back(src->hash);
    }

    lock_guard<recursive_mutex> lock(makerLock);
    const string name = mangledName(description, hashes)+"_kernel";

    Function *function = TheModule->getFunction(name);
    if (function != NULL)
        return executionEngine->getPointerToFunction(function);
    function = getFunction(name, hashes.size(), Type::getVoidTy(getGlobalContext()), Type::getInt32Ty(getGlobalContext()), Type::getInt32Ty(getGlobalContext()));

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
        {"initialize", lua_likely_initialize},
        {"clone", lua_likely_clone},
        {"delete", lua_likely_delete},
        {"allocate", lua_likely_allocate},
        {"free", lua_likely_free},
        {"write", lua_likely_write},
        {"encode", lua_likely_encode},
        {"decode", lua_likely_decode},
        {NULL, NULL}
    };

    // Register function metatable
    luaL_newmetatable(L, "likely_function");
    lua_pushstring(L, "__call");
    lua_pushcfunction(L, lua_likely__call);
    lua_settable(L, -3);

    // Idiom for registering library with member functions
    luaL_newmetatable(L, "likely");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, likely_members, 0);
    luaL_newlib(L, likely_globals);

    return 1;
}
