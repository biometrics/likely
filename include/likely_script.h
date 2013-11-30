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

#include <likely.h>

#ifdef __cplusplus
extern "C" {
#endif

// Contents of standard.likely
LIKELY_EXPORT const char *likely_standard_library();

struct lua_State;
LIKELY_EXPORT int luaopen_likely(lua_State *L);
LIKELY_EXPORT lua_State *likely_exec(const char *source, lua_State *L = NULL);
LIKELY_EXPORT likely_description likely_interpret(const char *source);
LIKELY_EXPORT void likely_stack_dump(lua_State *L, int levels = 1);

#ifdef __cplusplus
}
#endif

#endif // LIKELY_SCRIPT_H
