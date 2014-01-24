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

#include <iostream>
#include <sstream>
#include <vector>

#include "likely/likely_frontend.h"
#include "likely/likely_runtime.h"

using namespace std;

typedef struct likely_ast_private
{
    int ref_count;
} likely_ast_private;

likely_ast likely_new_atom(const char *atom, size_t begin, size_t end)
{
    const size_t atom_len = end - begin;
    likely_ast ast = (likely_ast) malloc(sizeof(likely_ast_struct) + sizeof(likely_ast_private) + atom_len + 1);
    ast->d_ptr = (likely_ast_private*) (ast + 1);
    ast->d_ptr->ref_count = 1;
    ast->atom = (const char*) (ast->d_ptr + 1);
    memcpy((void*) ast->atom, atom, atom_len);
    ((char*) ast->atom)[atom_len] = '\0';
    ast->atom_len = atom_len;
    ast->is_list = false;
    ast->begin = begin;
    ast->end = end;
    return ast;
}

likely_ast likely_new_list(likely_ast *atoms, size_t num_atoms)
{
    likely_ast ast = (likely_ast) malloc(sizeof(likely_ast_struct) + sizeof(likely_ast_private) + num_atoms * sizeof(likely_ast));
    ast->d_ptr = (likely_ast_private*) (ast + 1);
    ast->d_ptr->ref_count = 1;
    ast->atoms = (likely_ast*) (ast->d_ptr+1);
    memcpy(ast->atoms, atoms, num_atoms * sizeof(likely_ast));
    ast->num_atoms = num_atoms;
    ast->is_list = true;
    ast->begin = num_atoms == 0 ? 0 : atoms[0]->begin;
    ast->end = num_atoms == 0 ? 0 : atoms[num_atoms-1]->end;
    for (size_t i=0; i<ast->num_atoms; i++)
        likely_retain_ast(ast->atoms[i]);
    return ast;
}

likely_ast likely_retain_ast(likely_ast ast)
{
    if (!ast) return ast;
    ++ast->d_ptr->ref_count;
    return ast;
}

void likely_release_ast(likely_ast ast)
{
    if (!ast) return;
    if (--ast->d_ptr->ref_count != 0) return;
    if (ast->is_list)
        for (size_t i=0; i<ast->num_atoms; i++)
            likely_release_ast(ast->atoms[i]);
    free(ast);
}

static void tokenize(const char *str, const size_t len, vector<likely_ast> &tokens)
{
    size_t i = 0;
    while (i < len) {
        while (str[i] <= ' ') // ASCII whitespace and ignored characters
            i++;

        size_t begin = i;
        bool inString = false;
        while (((str[i] > ' ') && (str[i] != '(') && (str[i] != ')')) || inString) {
            if (str[i] == '"')
                inString = !inString;
            if (str[i] == '\\')
                i++;
            i++;
        }
        if (i == begin)
            i++;
        tokens.push_back(likely_new_atom(&str[begin], begin, i));
    }
}

// GFM = Github Flavored Markdown
static void tokenizeGFM(const char *str, const size_t len, vector<likely_ast> &tokens)
{
    bool inBlock = false, skipBlock = false;
    size_t lineStart = 0;
    while (lineStart < len) {
        size_t lineEnd = lineStart;
        while ((lineEnd < len) && (str[lineEnd] != '\n'))
            lineEnd++;
        const size_t lineLen = lineEnd - lineStart;

        if ((lineLen > 3) && !strncmp(&str[lineStart], "```", 3)) {
            // Found a code block marker
            if (skipBlock) {
                skipBlock = false;
            } else if (inBlock) {
                inBlock = false;
            } else if (str[3] != '\n') {
                // skip code blocks marked for specific languages
                skipBlock = true;
            } else {
                inBlock = true;
            }
        } else if (!skipBlock) {
            if (inBlock || ((lineLen > 4) && !strncmp(&str[lineStart], "    ", 4))) {
                // It's a code block
                tokenize(&str[lineStart], lineLen, tokens);
            } else {
                // Look for `inline code`
                size_t inlineStart = lineStart;
                do {
                    while ((inlineStart < lineEnd) && (str[inlineStart] != '`'))
                        inlineStart++;
                    size_t inlineEnd = inlineStart + 1;
                    while ((inlineEnd < lineEnd) && (str[inlineEnd] != '`'))
                        inlineEnd++;

                    if ((inlineStart < lineEnd) && (inlineEnd < lineEnd))
                        tokenize(&str[inlineStart], inlineEnd-inlineStart, tokens);

                    inlineStart = inlineEnd + 1;
                } while (inlineStart < lineEnd);
            }
        }

        lineStart = lineEnd + 1;
    }
}

likely_ast *likely_tokens_from_string(const char *str, size_t *num_tokens)
{
    static vector<likely_ast> tokens;
    likely_assert(str != NULL, "NULL 'str' argument in 'likely_tokens_from_string'");
    likely_assert(num_tokens != NULL, "NULL 'num_tokens' argument in 'likely_tokens_from_string'");

    for (size_t i=0; i<tokens.size(); i++)
        likely_release_ast(tokens[i]);
    tokens.clear();

    const size_t len = strlen(str);
    if (str[0] == '(') tokenize(str, len, tokens);
    else               tokenizeGFM(str, len, tokens);
    *num_tokens = tokens.size();
    return tokens.data();
}

static void print(const likely_ast ast, stringstream &stream)
{
    if (ast->is_list) {
        stream << "(";
        for (size_t i=0; i<ast->num_atoms; i++) {
            print(ast->atoms[i], stream);
            if (i != ast->num_atoms - 1)
                stream << " ";
        }
        stream << ")";
    } else {
        stream.write(ast->atom, ast->atom_len);
    }
}

const char *likely_tokens_to_string(const likely_ast *tokens, size_t num_tokens)
{
    static string str;
    likely_assert((tokens != NULL), "NULL 'tokens' argument in 'likely_tokens_to_string'");

    stringstream stream;
    for (size_t i=0; i<num_tokens; i++) {
        print(tokens[i], stream);
        stream << " ";
    }
    str = stream.str();
    return str.c_str();
}

static inline void check(likely_ast *tokens, size_t num_tokens, size_t offset)
{
    if (offset >= num_tokens)
        likely_throw(tokens[num_tokens-1], "unexpected end of expression");
}

static likely_ast parse(likely_ast *tokens, size_t num_tokens, size_t &offset)
{
    likely_ast start, end;
    vector<likely_ast> atoms;
    try {
        check(tokens, num_tokens, offset);
        start = tokens[offset++];
        if (start->atom[0] != '(')
            return start;

        check(tokens, num_tokens, offset);
        while (tokens[offset]->atom[0] != ')') {
            atoms.push_back(parse(tokens, num_tokens, offset));
            check(tokens, num_tokens, offset);
        }
        end = tokens[offset++];
    } catch (...) {
        return NULL;
    }

    return likely_new_list(atoms.data(), atoms.size());
}

likely_ast likely_ast_from_tokens(likely_ast *tokens, size_t num_tokens)
{
    size_t offset = 0;
    vector<likely_ast> expressions;
    while (offset < num_tokens)
        expressions.push_back(parse(tokens, num_tokens, offset));
    return likely_new_list(expressions.data(), expressions.size());
}

static void print(const likely_ast ast, vector<likely_ast> &tokens)
{
    if (ast->is_list) {
        tokens.push_back(likely_new_atom("(", 0, 1));
        for (size_t i=0; i<ast->num_atoms; i++)
            print(ast->atoms[i], tokens);
        tokens.push_back(likely_new_atom(")", 0, 1));
    } else {
        likely_retain_ast(ast);
        tokens.push_back(ast);
    }
}

likely_ast *likely_ast_to_tokens(const likely_ast ast, size_t *num_tokens)
{
    static vector<likely_ast> tokens;
    likely_assert((num_tokens != NULL), "NULL 'num_tokens' argument in 'likely_ast_to_tokens'");

    for (size_t i=0; i<tokens.size(); i++)
        likely_release_ast(tokens[i]);
    tokens.clear();

    print(ast, tokens);
    *num_tokens = tokens.size();
    return tokens.data();
}

likely_ast likely_ast_from_string(const char *expression)
{
    size_t num_tokens;
    likely_ast *tokens = likely_tokens_from_string(expression, &num_tokens);
    return likely_ast_from_tokens(tokens, num_tokens);
}

const char *likely_ast_to_string(const likely_ast ast)
{
    static string result;
    stringstream stream;
    print(ast, stream);
    result = stream.str();
    return result.c_str();
}

static likely_error_callback ErrorCallback = NULL;
static void *ErrorContext = NULL;

void likely_set_error_callback(likely_error_callback callback, void *context)
{
    ErrorCallback = callback;
    ErrorContext = context;
}

void likely_throw(likely_ast ast, const char *message)
{
    likely_error error;
    error.ast = ast;
    error.message = message;

    if (ErrorCallback) ErrorCallback(error, ErrorContext);
    else               likely_assert(false, message);

    throw error;
}
