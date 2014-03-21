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

#ifndef LIKELY_FRONTEND_H
#define LIKELY_FRONTEND_H

#include <stddef.h>
#include <likely/runtime.h>

struct likely_abstract_syntax_tree
{
    union {
        struct {
            struct likely_abstract_syntax_tree const **atoms;
            size_t num_atoms;
        };
        struct {
            const char *atom;
            size_t atom_len;
        };
    };

    size_t ref_count;
    size_t begin, end; // indicies into the source string
    bool is_list;
};
typedef struct likely_abstract_syntax_tree *likely_ast;
typedef struct likely_abstract_syntax_tree const *likely_const_ast;

typedef struct likely_error
{
    likely_const_ast where;
    const char *what;
} likely_error;
typedef void (*likely_error_callback)(likely_error error, void *context);

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

LIKELY_EXPORT likely_ast likely_new_atom(const char *str);
LIKELY_EXPORT likely_ast likely_new_atom_at(const char *str, size_t begin, size_t end);
LIKELY_EXPORT likely_ast likely_new_list(const likely_const_ast *atoms, size_t num_atoms);
LIKELY_EXPORT likely_ast likely_retain_ast(likely_const_ast ast);
LIKELY_EXPORT void likely_release_ast(likely_const_ast ast);

LIKELY_EXPORT likely_ast likely_tokens_from_string(const char *str, bool GFM);
LIKELY_EXPORT likely_ast likely_ast_from_tokens(likely_const_ast tokens);
LIKELY_EXPORT likely_ast likely_asts_from_tokens(likely_const_ast tokens); // Top level is a list of expressions
LIKELY_EXPORT likely_ast likely_ast_from_string(const char *str, bool GFM);
LIKELY_EXPORT likely_ast likely_asts_from_string(const char *str, bool GFM); // Top level is a list of expressions
LIKELY_EXPORT likely_mat likely_ast_to_string(likely_const_ast ast);

// Callback-style error handling
LIKELY_EXPORT void likely_set_error_callback(likely_error_callback callback, void *context);
LIKELY_EXPORT void likely_throw(likely_const_ast where, const char *what);
LIKELY_EXPORT likely_mat likely_error_to_string(likely_error error);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIKELY_FRONTEND_H
