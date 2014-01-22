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

#ifndef LIKELY_COMPILER_H
#define LIKELY_COMPILER_H

#include <likely/likely_runtime.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct likely_ast
{
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
    size_t start_pos, end_pos;
    bool is_list;
} likely_ast;

// If 'str' starts with '(' the string is assumed to contain s-expressions,
// otherwise 'str' is assumed to be Github Flavored Markdown (GFM) with s-expression(s) in the code blocks
LIKELY_EXPORT likely_ast *likely_tokens_from_string(const char *str, size_t *num_tokens); // Return value managed internally and guaranteed until the next call to this function
LIKELY_EXPORT const char *likely_tokens_to_string(likely_ast *ast, size_t num_tokens); // Return value managed internally and guaranteed until the next call to this function
LIKELY_EXPORT likely_ast likely_ast_from_tokens(likely_ast *tokens, size_t num_tokens); // Top level is a list of expressions
LIKELY_EXPORT likely_ast *likely_ast_to_tokens(const likely_ast ast, size_t *num_tokens); // Return value managed internally and guaranteed until the next call to this function
LIKELY_EXPORT likely_ast likely_ast_from_string(const char *str);
LIKELY_EXPORT const char *likely_ast_to_string(const likely_ast ast); // Return value managed internally and guaranteed until the next call to this function
LIKELY_EXPORT void likely_free_ast(likely_ast ast);

typedef likely_mat (*likely_function)(likely_const_mat, ...);
typedef likely_mat (*likely_function_n)(likely_const_mat*);
LIKELY_EXPORT likely_function likely_compile(likely_ast ast); // Takes ownership of ast
LIKELY_EXPORT likely_function_n likely_compile_n(likely_ast ast); // Takes ownership of ast
LIKELY_EXPORT void likely_compile_to_file(likely_ast ast, const char *symbol_name, likely_type *types, likely_arity n, const char *file_name, bool native); // Does _not_ take ownership of ast

LIKELY_EXPORT likely_mat likely_eval(likely_ast ast);

// Contents of library/standard.like
LIKELY_EXPORT extern const char likely_standard_library[];

#ifdef __cplusplus
}
#endif

#endif // LIKELY_COMPILER_H
