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

#ifndef LIKELY_SCRIPT_H
#define LIKELY_SCRIPT_H

#include <likely/likely_compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

struct lua_State;
LIKELY_EXPORT const char *likely_lua_to_string(struct lua_State *L);
LIKELY_EXPORT likely_ast likely_lua_to_ast(struct lua_State *L);
LIKELY_EXPORT void likely_lua_dump(struct lua_State *L, int levels);

// Contents of library/standard.like
LIKELY_EXPORT extern const char likely_standard_library[];

// Import Likely into a Lua runtime
LIKELY_EXPORT int luaopen_likely(struct lua_State *L);

// Execute a Likely script
LIKELY_EXPORT struct lua_State *likely_exec(const char *source, struct lua_State *L, int markdown);

// show() callback
typedef void (*likely_show_callback)(struct lua_State *L, void *context);
LIKELY_EXPORT void likely_set_show_callback(likely_show_callback callback, void *context);

#ifdef __cplusplus
}
#endif

#endif // LIKELY_SCRIPT_H
