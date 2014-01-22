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

static void tokenize(const char *str, const size_t len, vector<likely_ast> &tokens)
{
    size_t i = 0;
    while (i < len) {
        while (str[i] <= ' ') // ASCII whitespace and ignored characters
            i++;

        likely_ast token;
        token.is_list = false;
        token.start_pos = i;
        token.atom = &str[i];
        bool inString = false;
        while (((str[i] > ' ') && (str[i] != '(') && (str[i] != ')')) || inString) {
            if (str[i] == '"')
                inString = !inString;
            if (str[i] == '\\')
                i++;
            i++;
        }
        if (i == token.start_pos)
            i++;
        token.end_pos = i;
        token.atom_len = token.end_pos - token.start_pos;

        tokens.push_back(token);
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

    tokens.clear();
    const size_t len = strlen(str);
    if (str[0] == '(') tokenize(str, len, tokens);
    else               tokenizeGFM(str, len, tokens);
    *num_tokens = tokens.size();
    return tokens.data();
}

static void print(const likely_ast &ast, stringstream &stream)
{
    if (ast.is_list) {
        stream << "(";
        for (size_t i=0; i<ast.num_atoms; i++) {
            print(ast.atoms[i], stream);
            if (i != ast.num_atoms - 1)
                stream << " ";
        }
        stream << ")";
    } else {
        stream.write(ast.atom, ast.atom_len);
    }
}

const char *likely_tokens_to_string(likely_ast *tokens, size_t num_tokens)
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

static inline void checkTokens(size_t num_tokens, size_t offset)
{
    if (offset >= num_tokens)
        likely_assert(false, "unexpected end of expression");
}

static likely_ast parse(likely_ast *tokens, size_t num_tokens, size_t &offset)
{
    checkTokens(num_tokens, offset);
    const likely_ast &start = tokens[offset++];
    if (start.atom[0] != '(')
        return start;

    vector<likely_ast> atoms;
    checkTokens(num_tokens, offset);
    while (tokens[offset].atom[0] != ')') {
        atoms.push_back(parse(tokens, num_tokens, offset));
        checkTokens(num_tokens, offset);
    }
    const likely_ast &end = tokens[offset++];

    likely_ast ast;
    ast.is_list = true;
    ast.start_pos = start.start_pos;
    ast.end_pos = end.end_pos;
    ast.atoms = new likely_ast[atoms.size()];
    ast.num_atoms = atoms.size();
    memcpy(ast.atoms, atoms.data(), sizeof(likely_ast) * atoms.size());
    return ast;
}

likely_ast likely_ast_from_tokens(likely_ast *tokens, size_t num_tokens)
{
    size_t offset = 0;
    vector<likely_ast> expressions;
    while (offset < num_tokens)
        expressions.push_back(parse(tokens, num_tokens, offset));

    likely_ast ast;
    ast.is_list = true;
    ast.start_pos = expressions.empty() ? 0 : expressions.front().start_pos;
    ast.end_pos = expressions.empty() ? 0 : expressions.back().end_pos;
    ast.atoms = new likely_ast[expressions.size()];
    ast.num_atoms = expressions.size();
    memcpy(ast.atoms, expressions.data(), sizeof(likely_ast) * expressions.size());
    return ast;
}

static void print(const likely_ast &ast, vector<likely_ast> &tokens)
{
    if (ast.is_list) {
        likely_ast lParen;
        lParen.is_list = false;
        lParen.start_pos = 0;
        lParen.end_pos = 0;
        lParen.atom = "(";
        lParen.atom_len = 1;
        tokens.push_back(lParen);

        for (size_t i=0; i<ast.num_atoms; i++)
            print(ast.atoms[i], tokens);

        likely_ast rParen;
        rParen.is_list = false;
        rParen.start_pos = 0;
        rParen.end_pos = 0;
        rParen.atom = ")";
        rParen.atom_len = 1;
        tokens.push_back(rParen);
    } else {
        tokens.push_back(ast);
    }
}

likely_ast *likely_ast_to_tokens(const likely_ast ast, size_t *num_tokens)
{
    static vector<likely_ast> tokens;
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

void likely_free_ast(likely_ast ast)
{
    if (ast.is_list) {
        for (size_t i=0; i<ast.num_atoms; i++)
            likely_free_ast(ast.atoms[i]);
        delete[] ast.atoms;
    }
}
