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
#include <likely/likely_export.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct likely_ast
{
    struct likely_ast_private *d_ptr;
    union {
        struct {
            struct likely_ast *atoms;
            size_t num_atoms;
        };
        struct {
            const char *atom;
            size_t atom_len;
        };
    };

    size_t begin, end; // indicies into the source string
    bool is_list;
} likely_ast;

LIKELY_EXPORT likely_ast *likely_new_ast(int num_atoms, size_t begin, size_t end);

// If 'str' starts with '(' the string is assumed to contain s-expressions,
// otherwise 'str' is assumed to be Github Flavored Markdown (GFM) with s-expression(s) in the code blocks
LIKELY_EXPORT likely_ast *likely_tokens_from_string(const char *str, size_t *num_tokens); // Return value managed internally and guaranteed until the next call to this function
LIKELY_EXPORT const char *likely_tokens_to_string(likely_ast *tokens, size_t num_tokens); // Return value managed internally and guaranteed until the next call to this function
LIKELY_EXPORT likely_ast likely_ast_from_tokens(likely_ast *tokens, size_t num_tokens); // Top level is a list of expressions
LIKELY_EXPORT likely_ast *likely_ast_to_tokens(const likely_ast ast, size_t *num_tokens); // Return value managed internally and guaranteed until the next call to this function
LIKELY_EXPORT likely_ast likely_ast_from_string(const char *str);
LIKELY_EXPORT const char *likely_ast_to_string(const likely_ast ast); // Return value managed internally and guaranteed until the next call to this function
LIKELY_EXPORT void likely_free_ast(likely_ast ast);

typedef struct likely_error
{
    likely_ast ast; // where
    const char *message; //what
} likely_error;

typedef void (*likely_error_callback)(likely_error error, void *context);
LIKELY_EXPORT void likely_set_error_callback(likely_error_callback callback, void *context);

// Exception-style error handling
LIKELY_EXPORT void likely_throw(likely_ast token, const char *message);

#ifdef __cplusplus
}
#endif

#endif // LIKELY_FRONTEND_H
