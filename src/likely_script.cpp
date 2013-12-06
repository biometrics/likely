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

#include <cstring>
#include <sstream>
#include <vector>
#include <lua.hpp>

#include "likely_aux.h"
#include "likely_script.h"

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
    likely_const_mat m = checkLuaMat(L);
    lua_pushfstring(L, "Likely %dx%dx%dx%d %s %p", m->channels, m->columns, m->rows, m->frames, likely_type_to_string(m->type), m);
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

static int lua_likely_set(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 3, "'set' expected 3 arguments, got: %d", lua_gettop(L));
    likely_mat m = checkLuaMat(L);
    const char *field = lua_tostring(L, 2);
    int isnum;
    if      (!strcmp(field, "data"))          m->data = (likely_data*) lua_touserdata(L, 3);
    else if (!strcmp(field, "type"))        { m->type = (likely_type) lua_tointegerx(L, 3, &isnum); lua_likely_assert(L, isnum != 0, "'set' expected type to be an integer, got: %s", lua_tostring(L, 3)); }
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
    else                                      lua_likely_assert(L, false, "unrecognized field: %s", field);
    return 0;
}

static int lua_likely__newindex(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 3, "'__newindex' expected 3 arguments, got: %d", lua_gettop(L));
    return lua_likely_set(L);
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
    likely_data *data = NULL;
    int8_t copy = 0;

    int isnum;
    const int argc = lua_gettop(L);
    switch (argc) {
      case 7: copy     = lua_toboolean(L, 7);
      case 6: data     = (likely_data*) lua_touserdata(L, 6);
      case 5: frames   = lua_tointegerx(L, 5, &isnum); lua_likely_assert(L, isnum != 0, "'new' expected frames to be an integer, got: %s", lua_tostring(L, 5));
      case 4: rows     = lua_tointegerx(L, 4, &isnum); lua_likely_assert(L, isnum != 0, "'new' expected rows to be an integer, got: %s", lua_tostring(L, 4));
      case 3: columns  = lua_tointegerx(L, 3, &isnum); lua_likely_assert(L, isnum != 0, "'new' expected columns to be an integer, got: %s", lua_tostring(L, 3));
      case 2: channels = lua_tointegerx(L, 2, &isnum); lua_likely_assert(L, isnum != 0, "'new' expected channels to be an integer, got: %s", lua_tostring(L, 2));
      case 1: type     = (likely_type) lua_tointegerx(L, 1, &isnum); lua_likely_assert(L, isnum != 0, "'new' expected type to be an integer, got: %s", lua_tostring(L, 2));
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
    lua_likely_assert(L, lua_gettop(L) == 1, "'read' expected 1 argument, got: %d", lua_gettop(L));
    *newLuaMat(L) = likely_read(lua_tostring(L, 1));
    return 1;
}

static int lua_likely_write(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 2, "'write' expected 2 arguments, got: %d", lua_gettop(L));
    likely_write(checkLuaMat(L), lua_tostring(L, 2));
    return 0;
}

static int lua_likely_decode(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 1, "'decode' expected 1 argument, got: %d", lua_gettop(L));
    *newLuaMat(L) = likely_decode(checkLuaMat(L));
    return 1;
}

static int lua_likely_encode(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 2, "'write' expected 2 arguments, got: %d", lua_gettop(L));
    *newLuaMat(L) = likely_encode(checkLuaMat(L), lua_tostring(L, 2));
    return 1;
}

static int lua_likely_render(lua_State *L)
{
    lua_likely_assert(L, lua_gettop(L) == 1, "'render' expected 1 argument, got: %d", lua_gettop(L));
    *newLuaMat(L) = likely_render(checkLuaMat(L), NULL, NULL);
    return 1;
}

static int lua_likely_compile(lua_State *L)
{
    const int args = lua_gettop(L);
    lua_likely_assert(L, args >= 1, "'compile' expected at least one argument");

    vector<likely_type> types;
    for (int i=2; i<=args; i++)
        types.push_back(checkLuaMat(L, i)->type);

    // Compile the function
    lua_pushlightuserdata(L, likely_compile_n(lua_tostring(L, 1), (likely_arity)types.size(), types.data()));
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
    const int args = lua_gettop(L);
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
            lua_getglobal(L, "tostring");
            lua_pushvalue(L, 1);
            lua_call(L, 1, 1);
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
          default: dst = NULL; lua_likely_assert(L, false, "__call invalid arity: %d", mats.size());
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
        lua_call(L, (int)lua_objlen(L, argsIndex), 1);
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
    bool inCode = false;
    for (const string line : split(source, '\n')) {
        if (line == "```") {
            inCode = !inCode;
            continue;
        }

        if (inCode || (line.substr(0, 4) == "    "))
            result << line << "\n";
    }

    if (result.str().empty())
        result << source;
    return result.str();
}

lua_State *likely_exec(const char *source, lua_State *L)
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

    // Create a sandboxed enviornment
    lua_newtable(L); // _ENV
    lua_newtable(L); // metatable
    lua_getglobal(L, "_G");
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    if (luaL_loadstring(L, removeGFM(source).c_str())) return L;
    lua_pushvalue(L, -2);
    lua_setupvalue(L, -2, 1);
    lua_pcall(L, 0, LUA_MULTRET, 0);
    return L; // The sandboxed environment results are now on the top of the stack
}

likely_description likely_interpret(const char *source)
{
    static lua_State *L = NULL;
    stringstream command; command << "return tostring(" << source << ")";
    L = likely_exec(command.str().c_str(), L);
    return lua_tostring(L, -1);
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
