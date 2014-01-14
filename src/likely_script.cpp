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

#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <lua.hpp>

#ifdef LIKELY_IO
#include "likely/likely_io.h"
#endif

#include "likely/likely_script.h"

using namespace std;

static void lua_likely_assert(lua_State *L, bool condition, const char *format, ...)
{
    if (condition) return;
    va_list ap;
    va_start(ap, format);
    static const int errorBufferSize = 1024;
    static char errorBuffer[errorBufferSize];
    vsnprintf(errorBuffer, errorBufferSize, format, ap);
    luaL_error(L, "Likely %s.", errorBuffer);
}

static likely_mat checkLuaMat(lua_State *L, int index = 1)
{
    likely_mat *mp = (likely_mat*)luaL_checkudata(L, index, "likely");
    if (!mp) return NULL;
    return *mp;
}

static int lua_likely__tostring(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 1, "'__tostring' expected 1 argument, got: %d", lua_gettop(L));
    lua_pushstring(L, likely_print(checkLuaMat(L)));
    return 1;
}

static int lua_likely_get(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 2, "'get' expected 2 arguments, got: %d", lua_gettop(L));
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
    lua_likely_assert(L, lua_gettop(L) == 2, "'__index' expected 2 arguments, got: %d", lua_gettop(L));
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

static likely_type getType(lua_State *L, int index)
{
    int isnum;
    likely_type type = (likely_type) lua_tointegerx(L, index, &isnum);
    if (isnum)
        return type;

    if (lua_istable(L, index)) {
        lua_getfield(L, index, "type");
        type = getType(L, -1);
        lua_pop(L, 1);
        return type;
    }

    lua_likely_assert(L, false, "'getType' failure");
    return likely_type_null;
}

static int lua_likely_set(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 3, "'set' expected 3 arguments, got: %d", lua_gettop(L));
    likely_mat m = checkLuaMat(L);
    const char *field = lua_tostring(L, 2);
    int isnum;
    if      (!strcmp(field, "data"))          m->data = (likely_data) lua_touserdata(L, 3);
    else if (!strcmp(field, "type"))        { m->type = getType(L, 3); }
    else if (!strcmp(field, "channels"))    { m->channels  = lua_tointegerx(L, 3, &isnum); lua_likely_assert(L, isnum != 0, "'set' expected channels to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "columns"))     { m->columns   = lua_tointegerx(L, 3, &isnum); lua_likely_assert(L, isnum != 0, "'set' expected columns to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "rows"))        { m->rows      = lua_tointegerx(L, 3, &isnum); lua_likely_assert(L, isnum != 0, "'set' expected rows to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "frames"))      { m->frames    = lua_tointegerx(L, 3, &isnum); lua_likely_assert(L, isnum != 0, "'set' expected frames to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "depth"))       { likely_set_depth(&m->type, (int)lua_tointegerx(L, 3, &isnum)); lua_likely_assert(L, isnum != 0, "'set' expected depth to be an integer, got: %s", lua_tostring(L, 3)); }
    else if (!strcmp(field, "signed"))        likely_set_signed(&m->type, lua_toboolean(L, 3) != 0);
    else if (!strcmp(field, "floating"))      likely_set_floating(&m->type, lua_toboolean(L, 3) != 0);
    else if (!strcmp(field, "parallel"))      likely_set_parallel(&m->type, lua_toboolean(L, 3) != 0);
    else if (!strcmp(field, "heterogeneous")) likely_set_heterogeneous(&m->type, lua_toboolean(L, 3) != 0);
    else if (!strcmp(field, "multiChannel"))  likely_set_multi_channel(&m->type, lua_toboolean(L, 3) != 0);
    else if (!strcmp(field, "multiColumn"))   likely_set_multi_column(&m->type, lua_toboolean(L, 3) != 0);
    else if (!strcmp(field, "multiRow"))      likely_set_multi_row(&m->type, lua_toboolean(L, 3) != 0);
    else if (!strcmp(field, "multiFrame"))    likely_set_multi_frame(&m->type, lua_toboolean(L, 3) != 0);
    else if (!strcmp(field, "saturation"))    likely_set_saturation(&m->type, lua_toboolean(L, 3) != 0);
    else if (!strcmp(field, "reserved"))    { likely_set_reserved(&m->type, (int)lua_tointegerx(L, 3, &isnum)); lua_likely_assert(L, isnum != 0, "'set' expected reserved to be an integer, got: %s", lua_tostring(L, 3)); }
    else                                      lua_likely_assert(L, false, "'set' unrecognized field: %s", field);
    return 0;
}

static int lua_likely__newindex(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 3, "'__newindex' expected 3 arguments, got: %d", lua_gettop(L));
    return lua_likely_set(L);
}

static int lua_likely__eq(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 2, "'__eq' expected 2 arguments, got: %d", lua_gettop(L));
    lua_pushboolean(L, checkLuaMat(L, 1)->data == checkLuaMat(L, 2)->data);
    return 1;
}

static int lua_likely_elements(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 1, "'elements' expected 1 argument, got: %d", lua_gettop(L));
    lua_pushinteger(L, likely_elements(checkLuaMat(L)));
    return 1;
}

static int lua_likely_bytes(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 1, "'bytes' expected 1 argument, got: %d", lua_gettop(L));
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

static int lua_likely_new(lua_State *L)
{
    likely_type type = likely_type_f32;
    likely_size channels = 1;
    likely_size columns = 1;
    likely_size rows = 1;
    likely_size frames = 1;
    likely_data data = NULL;
    int8_t copy = 0;

    int isnum;
    const int argc = lua_gettop(L);
    switch (argc) {
      case 7: copy     = lua_toboolean(L, 7);
      case 6: data     = (likely_data) lua_touserdata(L, 6);
      case 5: frames   = lua_tointegerx(L, 5, &isnum); lua_likely_assert(L, isnum != 0, "'new' expected frames to be an integer, got: %s", lua_tostring(L, 5));
      case 4: rows     = lua_tointegerx(L, 4, &isnum); lua_likely_assert(L, isnum != 0, "'new' expected rows to be an integer, got: %s", lua_tostring(L, 4));
      case 3: columns  = lua_tointegerx(L, 3, &isnum); lua_likely_assert(L, isnum != 0, "'new' expected columns to be an integer, got: %s", lua_tostring(L, 3));
      case 2: channels = lua_tointegerx(L, 2, &isnum); lua_likely_assert(L, isnum != 0, "'new' expected channels to be an integer, got: %s", lua_tostring(L, 2));
      case 1: type     = getType(L, 1);
      case 0: break;
      default: lua_likely_assert(L, false, "'new' expected no more than 7 arguments, got: %d", argc);
    }

    *newLuaMat(L) = likely_new(type, channels, columns, rows, frames, data, copy);
    return 1;
}

static int lua_likely_scalar(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 1, "'scalar' expected 1 argument, got: %d", lua_gettop(L));
    int isnum;
    double value = lua_tonumberx(L, 1, &isnum);
    lua_likely_assert(L, isnum != 0, "'scalar' expected a numeric argument, got: %s", lua_tostring(L, 1));
    *newLuaMat(L) = likely_scalar(value);
    return 1;
}

static int lua_likely_copy(lua_State *L)
{
    const int args = lua_gettop(L);
    bool copy_data = false;
    likely_mat m = NULL;
    switch (args) {
        case 2: copy_data = (lua_toboolean(L, 2) != 0);
        case 1: m = checkLuaMat(L);
        case 0: break;
        default: lua_likely_assert(L, false, "'copy' expected 1-2 arguments, got: %d", args);
    }
    *newLuaMat(L) = likely_copy(m, copy_data);
    return 1;
}

static int lua_likely__gc(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 1, "'__gc' expected 1 argument, got: %d", lua_gettop(L));
    likely_release(checkLuaMat(L));
    return 0;
}

static int lua_likely_read(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 1, "'read' expected 1 argument, got: %d", args);
#ifdef LIKELY_IO
    likely_mat m = likely_read(lua_tostring(L, 1));
    if (m == NULL)
        luaL_error(L, "Likely 'read' failure.");
#else
    likely_mat m = NULL;
    luaL_error(L, "Likely 'read' unsupported.");
#endif
    if (m->type == likely_type_null)
        luaL_error(L, "Likely 'read' failed on %s.", lua_tostring(L, 1));
    *newLuaMat(L) = m;
    return 1;
}

static int lua_likely_write(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 2, "'write' expected 2 arguments, got: %d", args);
#ifdef LIKELY_IO
    likely_write(checkLuaMat(L), lua_tostring(L, 2));
#else
    luaL_error(L, "Likely 'write' unsupported.");
#endif
    return 0;
}

static int lua_likely_decode(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 1, "'decode' expected 1 argument, got: %d", args);
#ifdef LIKELY_IO
    likely_mat m = likely_decode(checkLuaMat(L));
    if (m == NULL)
        luaL_error(L, "Likely 'decode' failure.");
    *newLuaMat(L) = m;
#else
    luaL_error(L, "Likely 'decode' unsupported.");
#endif
    return 1;
}

static int lua_likely_encode(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 2, "'encode' expected 2 arguments, got: %d", args);
#ifdef LIKELY_IO
    likely_mat m = likely_encode(checkLuaMat(L), lua_tostring(L, 2));
    if (m == NULL)
        luaL_error(L, "Likely 'encode' failure.");
    *newLuaMat(L) = m;
#else
    luaL_error(L, "Likely 'encode' unsupported.");
#endif
    return 1;
}

static int lua_likely_render(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 1, "'render' expected 1 argument, got: %d", lua_gettop(L));
#ifdef LIKELY_IO
    *newLuaMat(L) = likely_render(checkLuaMat(L), NULL, NULL);
#else
    luaL_error(L, "likely 'render' unsupported.");
#endif
    return 1;
}

static likely_show_callback ShowCallback = NULL;
static void *ShowContext = NULL;

static int lua_likely_show(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 2, "'show' expected 2 arguments, got: %d", args);
    if (ShowCallback) ShowCallback(L, ShowContext);
    return 0;
}

static void copyRecursive(lua_State *src, lua_State *dst)
{
    const int type = lua_type(src, -1);
    if (type == LUA_TBOOLEAN) {
        lua_pushboolean(dst, lua_toboolean(src, -1));
    } else if (type == LUA_TNUMBER) {
        lua_pushnumber(dst, lua_tonumber(src, -1));
    } else if (type == LUA_TSTRING) {
        lua_pushstring(dst, lua_tostring(src, -1));
    } else if (type == LUA_TTABLE) {
        lua_newtable(dst);
        lua_pushnil(src);
        while (lua_next(src, -2)) {
            lua_pushvalue(src, -2);
            copyRecursive(src, dst); // copy key
            lua_pop(src, 1);
            copyRecursive(src, dst); // copy value
            lua_pop(src, 1);
            lua_settable(dst, -3);
        }
    } else {
        likely_assert(false, "'copyRecursive' unsupported type");
    }
}

static int lua_likely_expression(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 1, "'expression' expected one argument, got: %d", args);
    lua_newtable(L);
    lua_setmetatable(L, -2);
    return 1;
}

static void replaceRecursive(lua_State *L)
{
    int i = 1;
    bool done = false;
    while (!done) {
        lua_pushinteger(L, i);
        lua_gettable(L, -2);

        if (lua_isnil(L, -1)) {
            done = true;
        } else if (lua_istable(L, -1)) {
            replaceRecursive(L);
        } else if (lua_equal(L, -1, 1)) {
            lua_pushinteger(L, i);
            lua_pushvalue(L, 2);
            lua_settable(L, -4);
        }

        lua_pop(L, 1);
        i++;
    }
}

static int lua_likely_replace(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 3, "'replace' expected three arguments, got: %d", args);
    replaceRecursive(L);
    return 1;
}

static int lua_likely_compile(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 1, "'compile' expected one argument, got: %d", args);

    // Retrieve or compile the function
    static map<string,likely_function_n> functions;
    const string source = likely_ir_to_string(L);
    map<string,likely_function_n>::const_iterator it = functions.find(source);
    if (it == functions.end()) {
        likely_ir ir = luaL_newstate();
        copyRecursive(L, ir);
        functions.insert(pair<string,likely_function_n>(source, likely_compile_n(likely_ir_to_ast(ir))));
        it = functions.find(source);
    }

    // Return a closure
    lua_getglobal(L, "closure");
    lua_newtable(L); // arguments
    lua_pushinteger(L, 1);
    lua_pushlightuserdata(L, (void*)it->second);
    lua_settable(L, -3);
    lua_newtable(L); // parameters
    lua_pushinteger(L, 1);
    lua_pushstring(L, "Likely JIT function");
    lua_settable(L, -3);
    lua_call(L, 2, 1);
    return 1;
}

static int lua_likely_closure(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 2, "'closure' expected 2 arguments, got: %d", args);

    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_setfield(L, -2, "parameters");

    // Construct parameter LUT
    lua_newtable(L);
    lua_pushnil(L);
    int index = 1, newIndex = 2, lastUnset = 1;
    while (lua_next(L, 2)) {
        if (index == 1) { // skip function
            lua_pop(L, 1);
        } else {
            // All parameters can be set by name
            lua_pushvalue(L, -2);
            lua_settable(L, -4);

            // Unassigned parameters can also be set by index
            lua_pushvalue(L, -1);
            lua_gettable(L, 1);
            if (lua_isnil(L, -1)) {
                lua_pushinteger(L, newIndex);
                lua_pushvalue(L, -3);
                lua_settable(L, -5);
                lastUnset = index;
                newIndex++;
            }
            lua_pop(L, 1);
        }

        index++;
    }

    // Assigned parameters after the last unassigned parameter can be set by index
    for (int i=lastUnset+1; i<index; i++) {
        lua_pushinteger(L, newIndex);
        lua_pushinteger(L, i);
        lua_settable(L, -3);
        newIndex++;
    }
    lua_setfield(L, -2, "parameterLUT");

    luaL_getmetatable(L, "likely_closure");
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_likely__call(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args >= 1, "'__call' expected at least one argument");

    // Copy the closure
    lua_newtable(L);
    lua_pushnil(L);
    while (lua_next(L, 1)) {
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_settable(L, -4);
    }

    // Using {} syntax?
    bool curry = (args == 2) && lua_istable(L, 2);
    if (curry && lua_getmetatable(L, 2)) {
        // Make sure it isn't a class
        curry = false;
        lua_pop(L, 1);
    }

    // Add the new arguments
    lua_getfield(L, 1, "parameterLUT");
    if (curry) {
        lua_pushnil(L);
        while (lua_next(L, 2)) {
            if (lua_isnumber(L, -2)) lua_pushinteger(L, lua_tointeger(L, -2) + 1);
            else                     lua_pushvalue(L, -2);
            lua_gettable(L, -4);
            lua_insert(L, -2);
            lua_settable(L, -5);
        }
    } else {
        for (int i=2; i<=args; i++) {
            lua_pushinteger(L, i);
            lua_gettable(L, -2);
            if (lua_isnil(L, -1)) {
                // Append extra arguments
                lua_pop(L, 1);
                lua_pushinteger(L, luaL_len(L, -2) + 1);
            }
            lua_pushvalue(L, i);
            lua_settable(L, -4);
        }
    }
    lua_pop(L, 1);

    if (curry) {
        // Return a new closure
        lua_getglobal(L, "closure");
        lua_pushvalue(L, -2); // arguments
        lua_getfield(L, 1, "parameters");
        lua_call(L, 2, 1);
        return 1;
    }

    // Ensure all the arguments are provided
    lua_getfield(L, 1, "parameters");
    const int parameters = luaL_len(L, -2);
    if (parameters < luaL_len(L, -1))
        luaL_error(L, "Insufficient arguments!");
    lua_pop(L, 1);

    // Unroll the arguments
    const int closureIndex = lua_gettop(L);
    for (int i=1; i<=parameters; i++) {
        lua_pushinteger(L, i);
        lua_gettable(L, closureIndex);
    }

    // Call the function
    if (lua_isuserdata(L, closureIndex+1)) {
        // JIT
        vector<likely_const_mat> mats;
        for (int i=2; i<=parameters; i++)
            mats.push_back(checkLuaMat(L, closureIndex+i));
        *newLuaMat(L) = reinterpret_cast<likely_function_n>(lua_touserdata(L, closureIndex+1))(mats.data());
    } else {
        // Regular
        const bool core = (lua_iscfunction(L, closureIndex+1) != 0);
        lua_call(L, parameters-1, 1);
        if (!core && lua_istable(L, -1)) {
            // Assume an expression was created
            luaL_getmetatable(L, "likely_expression");
            lua_setmetatable(L, -2);
        }
    }

    return 1;
}

static int lua_likely__concat(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 2, "'__concat' expected two arguments, got: %d", args);
    lua_getglobal(L, "closure");

    // arguments
    lua_newtable(L);
    lua_pushinteger(L, 1);
    lua_getglobal(L, "chain");
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_call(L, 2, 1);
    lua_settable(L, -3);

    // parameters
    lua_newtable(L);
    lua_pushinteger(L, 1);
    lua_pushstring(L, "chain");
    lua_settable(L, -3);
    lua_pushinteger(L, 2);
    lua_pushstring(L, "operand");
    lua_settable(L, -3);

    // return a new closure
    lua_call(L, 2, 1);
    return 1;
}

static int lua_likely_expression__call(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 2, "'expression__call' expected exactly two arguments");

    lua_pushnil(L);
    while (lua_next(L, -2)) {
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_settable(L, 1);
    }

    lua_pushvalue(L, 1);
    return 1;
}

static void findMats(lua_State *L, vector<likely_mat> &mats)
{
    int i = 1;
    bool done = false;
    while (!done) {
        lua_pushinteger(L, i);
        lua_gettable(L, -2);

        if (lua_isnil(L, -1)) {
            done = true;
        } else if (lua_istable(L, -1)) {
            findMats(L, mats);
        } else if (likely_mat *mat = (likely_mat*)luaL_testudata(L, -1, "likely")) {
            if (find(mats.begin(), mats.end(), *mat) == mats.end())
                mats.push_back(*mat);
        }

        lua_pop(L, 1);
        i++;
    }
}

static int lua_likely_global(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 2, "'global' expected two arguments, got: %d", args);

    lua_getfield(L, 1, "_L");
    lua_pushvalue(L, 2);
    lua_gettable(L, -2);

    if (lua_isnil(L, -1)) {
        lua_getfield(L, 1, "_G");
        lua_pushvalue(L, 2);
        lua_gettable(L, -2);
    }

    if (lua_isnil(L, -1)) {
        // Construct an expression
        lua_newtable(L);
        lua_pushinteger(L, 1);
        lua_pushvalue(L, 2);
        lua_settable(L, -3);
        luaL_getmetatable(L, "likely_expression");
        lua_setmetatable(L, -2);
    }

    return 1;
}

static int lua_likely_new_global(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args == 3, "'new_global' expected three arguments, got: %d", args);

    bool expression = false;
    if (lua_istable(L, 3) && lua_getmetatable(L, 3)) {
        luaL_getmetatable(L, "likely_expression");
        expression = (lua_rawequal(L, -1, -2) != 0);
        lua_pop(L, 2);
    }

    if (expression) {
        vector<likely_mat> mats;
        findMats(L, mats);

        for (size_t i=0; i<mats.size(); i++) {
            lua_getglobal(L, "replace");
            *newLuaMat(L) = mats[i];
            likely_retain(mats[i]);
            lua_getglobal(L, "arg");
            lua_pushinteger(L, i);
            lua_call(L, 1, 1);
            lua_pushvalue(L, -4);
            lua_call(L, 3, 0);
        }

        lua_getglobal(L, "compile");
        lua_insert(L, -2);
        lua_call(L, 1, 1);

        for (size_t i=0; i<mats.size(); i++) {
            *newLuaMat(L) = mats[i];
            likely_retain(mats[i]);
        }
        lua_call(L, (int) mats.size(), 1);
    }

    // Assign it to the proxy table
    lua_getfield(L, 1, "_L");
    lua_pushvalue(L, -3);
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    return 0;
}

int luaopen_likely(lua_State *L)
{
    static const struct luaL_Reg likely_globals[] = {
        {"new", lua_likely_new},
        {"scalar", lua_likely_scalar},
        {"read", lua_likely_read},
        {"closure", lua_likely_closure},
        {"compile", lua_likely_compile},
        {"expression", lua_likely_expression},
        {"replace", lua_likely_replace},
        {"show", lua_likely_show},
        {NULL, NULL}
    };

    static const struct luaL_Reg likely_members[] = {
        {"__index", lua_likely__index},
        {"__newindex", lua_likely__newindex},
        {"__eq", lua_likely__eq},
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
        {NULL, NULL}
    };

    static const struct luaL_Reg likely_expression[] = {
        {"__call", lua_likely_expression__call},
        {NULL, NULL}
    };

    // Register closure metatable
    luaL_newmetatable(L, "likely_closure");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, likely_closure, 0);

    // Register expression metatable
    luaL_newmetatable(L, "likely_expression");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, likely_expression, 0);

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

vector<string> split(const string &s, char delim) {
    vector<string> elems;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim))
        elems.push_back(item);
    return elems;
}

// GFM = Github Flavored Markdown
static string removeGFM(const string &source)
{
    stringstream result;
    bool inCode = false, skipBlock = false;
    const string codeBlock = "```";
    for (const string line : split(source, '\n')) {
        if (line.substr(0, 3) == codeBlock) {
            const string syntax = line.substr(line.size()-3, 3);
            if (skipBlock) {
                skipBlock = false;
            } else if ((syntax != codeBlock) &&
                       (syntax != "Lua") &&
                       (syntax != "lua")) {
                skipBlock = true;
            } else {
                inCode = !inCode;
            }
            continue;
        }

        if (inCode && (line.find('`') == string::npos)) {
            result << line << "\n";
        } else if (!skipBlock && (line.substr(0, 4) == "    ")) {
            result << line.substr(4, line.size()-4) << "\n";
        } else {
            string::size_type start = -1, stop = -1;
            do {
                start = line.find('`', stop  + 1);
                stop  = line.find('`', start + 1);
                if ((start != string::npos) && (stop != string::npos))
                    result << line.substr(start + 1, stop - start - 1) << "\n";
                else
                    break;
            } while (true);
        }
    }

    return result.str();
}

lua_State *likely_exec(const char *source, lua_State *L, int markdown)
{
    if (L == NULL) {
        L = luaL_newstate();
        luaL_openlibs(L);
        luaL_requiref(L, "likely", luaopen_likely, 1);
        lua_pop(L, 1);
        luaL_dostring(L, removeGFM(likely_standard_library).c_str());
    }

    // Clear the previous stack
    lua_settop(L, 0);

    // Create a sandboxed environment
    lua_newtable(L); // _ENV
    lua_getglobal(L, "_G");
    lua_setfield(L, -2, "_G");
    lua_newtable(L); // proxy
    lua_setfield(L, -2, "_L");
    lua_newtable(L); // metatable
    lua_pushcfunction(L, lua_likely_global);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, lua_likely_new_global);
    lua_setfield(L, -2, "__newindex");
    lua_setmetatable(L, -2);

    if (luaL_loadstring(L, markdown ? removeGFM(source).c_str() : source)) return L;
    lua_pushvalue(L, -2);
    lua_setupvalue(L, -2, 1);
    lua_pcall(L, 0, LUA_MULTRET, 0);
    return L; // The sandboxed environment is now on the top of the stack
}

likely_ir likely_ir_from_expression(const char *expression)
{
    static lua_State *L = NULL;
    stringstream command; command << "return " << expression;
    L = likely_exec(command.str().c_str(), L, 0);
    likely_assert(lua_istable(L, -1), "'likely_ir_from_expression' expected a table result");

    // Return the result in a new state
    likely_ir ir = luaL_newstate();
    copyRecursive(L, ir);
    return ir;
}

LIKELY_EXPORT void likely_set_show_callback(likely_show_callback callback, void *context)
{
    ShowCallback = callback;
    ShowContext = context;
}
