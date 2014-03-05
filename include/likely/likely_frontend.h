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
#include <likely/likely_runtime.h>

#ifdef __cplusplus
extern "C" {
#endif

struct likely_abstract_syntax_tree
{
    struct likely_abstract_syntax_tree_private *d_ptr;
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

    size_t begin, end; // indicies into the source string
    bool is_list;
};
typedef struct likely_abstract_syntax_tree *likely_ast;
typedef struct likely_abstract_syntax_tree const *likely_const_ast;

LIKELY_EXPORT likely_ast likely_new_atom(const char *str, size_t begin, size_t end);
LIKELY_EXPORT likely_ast likely_new_list(likely_const_ast *atoms, size_t num_atoms);
LIKELY_EXPORT likely_ast likely_retain_ast(likely_const_ast ast);
LIKELY_EXPORT void likely_release_ast(likely_const_ast ast);

// If 'str' starts with '(' the string is assumed to contain s-expressions,
// otherwise 'str' is assumed to be Github Flavored Markdown (GFM) with s-expression(s) in the code blocks
LIKELY_EXPORT likely_ast likely_tokens_from_string(const char *str);
LIKELY_EXPORT likely_ast likely_ast_from_tokens(likely_const_ast tokens);
LIKELY_EXPORT likely_ast likely_ast_from_tokens_at(likely_const_ast tokens, size_t *offset);
LIKELY_EXPORT likely_ast likely_ast_from_string(const char *str);
LIKELY_EXPORT likely_ast likely_asts_from_string(const char *str); // Top level is a list of expressions
LIKELY_EXPORT const char *likely_ast_to_string(const likely_const_ast ast); // Return value managed internally and guaranteed until the next call to this function

typedef struct likely_error
{
    likely_const_ast ast; // where
    const char *message; //what
} likely_error;

typedef void (*likely_error_callback)(likely_error error, void *context);
LIKELY_EXPORT void likely_set_error_callback(likely_error_callback callback, void *context);

// Callback-style error handling
LIKELY_EXPORT void likely_throw(likely_const_ast token, const char *message);

#ifdef __cplusplus
}
#endif

#endif // LIKELY_FRONTEND_H
