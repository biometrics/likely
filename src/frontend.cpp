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

#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

#include "likely/frontend.h"
#include "likely/runtime.h"

using namespace std;

likely_ast likely_new_atom(const char *str, size_t begin, size_t end)
{
    const size_t atom_len = end - begin;
    likely_ast ast = (likely_ast) malloc(sizeof(likely_abstract_syntax_tree) + atom_len + 1);
    ast->atom = (const char*) (ast + 1);
    memcpy((void*) ast->atom, &str[begin], atom_len);
    ((char*) ast->atom)[atom_len] = '\0';
    ast->atom_len = atom_len;
    ast->is_list = false;
    ast->ref_count = 1;
    ast->begin = begin;
    ast->end = end;
    return ast;
}

likely_ast likely_new_list(likely_const_ast *atoms, size_t num_atoms)
{
    likely_ast ast = (likely_ast) malloc(sizeof(likely_abstract_syntax_tree) + num_atoms * sizeof(likely_const_ast));
    ast->atoms = (likely_const_ast*) (ast+1);
    memcpy(ast->atoms, atoms, num_atoms * sizeof(likely_const_ast));
    ast->num_atoms = num_atoms;
    ast->is_list = true;
    ast->ref_count = 1;
    ast->begin = num_atoms == 0 ? 0 : atoms[0]->begin;
    ast->end = num_atoms == 0 ? 0 : atoms[num_atoms-1]->end;
    return ast;
}

likely_ast likely_retain_ast(likely_const_ast ast)
{
    if (ast) ++const_cast<likely_ast>(ast)->ref_count;
    return (likely_ast) ast;
}

void likely_release_ast(likely_const_ast ast)
{
    if (!ast || --const_cast<likely_ast>(ast)->ref_count) return;
    if (ast->is_list)
        for (size_t i=0; i<ast->num_atoms; i++)
            likely_release_ast(ast->atoms[i]);
    free((void*) ast);
}

static void tokenize(const char *str, const size_t len, vector<likely_const_ast> &tokens)
{
    size_t i = 0;
    while (true) {
        // Skip whitespace and control characters
        const char ignored = ' ';
        while ((i < len) && (str[i] <= ignored))
            i++;
        if (i == len) break;

        // Skip comments
        if (str[i] == ';') {
            while ((i < len) && (str[i] != '\n'))
                i++;
            i++;
            if (i >= len) break;
        }

        size_t begin = i;
        bool inString = false;
        while ((i < len) && (inString || ((str[i] > ignored) && (str[i] != '(') && (str[i] != ')') && (str[i] != ';')))) {
            if      (str[i] == '"')  inString = !inString;
            else if (str[i] == '\\') i++;
            i++;
        }
        if (i == begin)
            i++;

        tokens.push_back(likely_new_atom(str, begin, i));
    }
}

// GFM = Github Flavored Markdown
static void tokenizeGFM(const char *str, const size_t len, vector<likely_const_ast> &tokens)
{
    bool inBlock = false, skipBlock = false;
    size_t lineStart = 0;
    while (lineStart < len) {
        size_t lineEnd = lineStart;
        while ((lineEnd < len) && (str[lineEnd] != '\n'))
            lineEnd++;
        const size_t lineLen = lineEnd - lineStart;

        if ((lineLen >= 3) && !strncmp(&str[lineStart], "```", 3)) {
            // Found a code block marker
            if (skipBlock) {
                skipBlock = false;
            } else if (inBlock) {
                inBlock = false;
            } else if (lineLen > 3) {
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
                size_t inlineStart = lineStart+1;
                do {
                    while ((inlineStart < lineEnd) && (str[inlineStart-1] != '`'))
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

static likely_ast cleanup(vector<likely_const_ast> &atoms)
{
    for (size_t i=0; i<atoms.size(); i++)
        likely_release_ast(atoms[i]);
    atoms.clear();
    return NULL;
}

likely_ast likely_tokens_from_string(const char *str)
{
    if (!str) return NULL;
    vector<likely_const_ast> tokens;
    const size_t len = strlen(str);
    if (str[0] == '(') tokenize(str, len, tokens);
    else               tokenizeGFM(str, len, tokens);
    return likely_new_list(tokens.data(), tokens.size());
}

likely_ast likely_ast_from_tokens(likely_const_ast tokens)
{
    size_t offset = 0;
    likely_ast ast = likely_ast_from_tokens_at(tokens, &offset);
    if (offset < tokens->num_atoms) {
        likely_throw(tokens->atoms[offset], "tokens leftover after parsing");
        likely_release_ast(ast);
        return NULL;
    }
    return ast;
}

likely_ast likely_ast_from_tokens_at(likely_const_ast tokens, size_t *offset)
{
    likely_const_ast start = tokens->atoms[(*offset)++];
    if (strcmp(start->atom, "(")) {
        if (!strcmp(start->atom, ")")) {
            likely_throw(start, "extra ')'");
            return NULL;
        }
        return likely_retain_ast(start);
    }

    vector<likely_const_ast> atoms;
    do {
        if (*offset >= tokens->num_atoms) {
            likely_throw(tokens->atoms[tokens->num_atoms-1], "missing ')'");
            return cleanup(atoms);
        }
        if (!strcmp(tokens->atoms[*offset]->atom, ")"))
            break;
        likely_const_ast atom = likely_ast_from_tokens_at(tokens, offset);
        if (atom == NULL)
            return cleanup(atoms);
        atoms.push_back(atom);
    } while (true);
    likely_const_ast end = tokens->atoms[(*offset)++];

    likely_ast list = likely_new_list(atoms.data(), atoms.size());
    list->begin = start->begin;
    list->end = end->end;
    return list;
}

likely_ast likely_ast_from_string(const char *str)
{
    likely_const_ast tokens = likely_tokens_from_string(str);
    likely_ast ast = likely_ast_from_tokens(tokens);
    likely_release_ast(tokens);
    return ast;
}

likely_ast likely_asts_from_string(const char *str)
{
    likely_const_ast tokens = likely_tokens_from_string(str);
    size_t offset = 0;
    vector<likely_const_ast> expressions;
    while (offset < tokens->num_atoms) {
        likely_const_ast expression = likely_ast_from_tokens_at(tokens, &offset);
        if (expression == NULL)
            return cleanup(expressions);
        expressions.push_back(expression);
    }
    likely_release_ast(tokens);
    return likely_new_list(expressions.data(), expressions.size());
}

static void print(const likely_const_ast ast, stringstream &stream)
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

likely_mat likely_ast_to_string(likely_const_ast ast)
{
    stringstream stream;
    print(ast, stream);
    return likely_string(stream.str().c_str());
}

static likely_error_callback ErrorCallback = NULL;
static void *ErrorContext = NULL;

void likely_set_error_callback(likely_error_callback callback, void *context)
{
    ErrorCallback = callback;
    ErrorContext = context;
}

void likely_throw(likely_const_ast ast, const char *message)
{
    if (ErrorCallback) {
        likely_error error;
        error.ast = ast;
        error.message = message;
        ErrorCallback(error, ErrorContext);
    } else {
        stringstream msg;
        msg << message << ": " << likely_ast_to_string(ast) << " at position: " << ast->begin;
        likely_assert(false, msg.str().c_str());
    }
}