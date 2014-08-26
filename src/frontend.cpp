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

#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <vector>

#include "likely/frontend.h"
#include "likely/runtime.h"

using namespace std;

likely_ast likely_new_atom(const char *str, likely_size len)
{
    likely_ast ast = (likely_ast) malloc(sizeof(likely_abstract_syntax_tree) + len + 1);
    const_cast<const char*&>(ast->atom) = reinterpret_cast<const char*>(ast + 1);
    memcpy((void*) ast->atom, str, len);
    ((char*) ast->atom)[len] = '\0';
    ast->atom_len = len;
    ast->parent = NULL;
    ast->ref_count = 1;
    ast->begin_line = 0;
    ast->begin_column = 0;
    ast->end_line = 0;
    ast->end_column = 0;
    ast->type = likely_ast_unknown;
    return ast;
}

likely_ast likely_new_list(const likely_ast *atoms, likely_size num_atoms)
{
    likely_ast ast = (likely_ast) malloc(sizeof(likely_abstract_syntax_tree) + num_atoms * sizeof(likely_ast));
    const_cast<const likely_ast*&>(ast->atoms) = reinterpret_cast<likely_ast*>(ast+1);
    memcpy(const_cast<likely_ast*>(ast->atoms), atoms, num_atoms * sizeof(likely_ast));
    ast->num_atoms = num_atoms;
    ast->parent = NULL;
    ast->ref_count = 1;
    ast->begin_line = (num_atoms == 0) ? 0 : atoms[0]->begin_line;
    ast->begin_column = (num_atoms == 0) ? 0 : atoms[0]->begin_column;
    ast->end_line = (num_atoms == 0) ? 0 : atoms[num_atoms-1]->end_line;
    ast->end_column = (num_atoms == 0) ? 0 : atoms[num_atoms-1]->end_column;
    ast->type = likely_ast_list;
    for (likely_size i=0; i<num_atoms; i++)
        ast->atoms[i]->parent = likely_retain_ast(ast);
    return ast;
}

likely_ast likely_copy_ast(likely_const_ast ast)
{
    likely_ast copy;
    if (ast->type == likely_ast_list) {
        vector<likely_ast> atoms;
        for (size_t i=0; i<ast->num_atoms; i++)
            atoms.push_back(likely_copy_ast(ast->atoms[i]));
        copy = likely_new_list(atoms.data(), atoms.size());
    } else {
        copy = likely_new_atom(ast->atom, ast->atom_len);
    }
    copy->parent = likely_retain_ast(ast->parent);
    copy->begin_line = ast->begin_line;
    copy->begin_column = ast->begin_column;
    copy->end_line = ast->end_line;
    copy->end_column = ast->end_column;
    copy->type = ast->type;
    return copy;
}

likely_ast likely_retain_ast(likely_const_ast ast)
{
    if (ast) ++const_cast<likely_ast>(ast)->ref_count;
    return const_cast<likely_ast>(ast);
}

void likely_release_ast(likely_const_ast ast)
{
    if (!ast || --const_cast<likely_ast>(ast)->ref_count) return;
    if (ast->type == likely_ast_list)
        for (size_t i=0; i<ast->num_atoms; i++)
            likely_release_ast(ast->atoms[i]);
    likely_release_ast(ast->parent);
    free((void*) ast);
}

static void incrementCounters(char c, likely_size &line, likely_size &column)
{
    if (c == '\n') {
        line = line + 1;
        column = 0;
    } else {
        column++;
    }
}

static void tokenize(const char *str, const size_t len, vector<likely_ast> &tokens, likely_size line, likely_size column)
{
    size_t i = 0;
    while (true) {
        // Skip whitespace and control characters
        const char ignored = ' ';
        while ((i < len) && (str[i] <= ignored)) {
            incrementCounters(str[i], line, column);
            i++;
        }
        if (i == len)
            break;

        size_t begin = i;
        const likely_size begin_line = line;
        const likely_size begin_column = column;
        bool inString = false;
        while ((i < len) && (inString || ((str[i] > ignored) && (str[i] != '(')
                                                             && (str[i] != ')')
                                                             && (str[i] != '.')
                                                             && (str[i] != ':')
                                                             && (str[i] != ';')))) {
            incrementCounters(str[i], line, column);
            if      (str[i] == '"')  inString = !inString;
            else if (str[i] == '\\') { i++; column++; }
            i++;
        }
        if (i == begin) {
            i++;
            column++;
        }

        likely_ast token = likely_new_atom(&str[begin], i-begin);
        token->begin_line = begin_line;
        token->begin_column = begin_column;
        token->end_line = line;
        token->end_column = column;
        tokens.push_back(token);
    }
}

// GFM = Github Flavored Markdown
static void tokenizeGFM(const char *str, const size_t len, vector<likely_ast> &tokens)
{
    likely_size line = 0;
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
                tokenize(&str[lineStart], lineLen, tokens, line, 0);
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
                        tokenize(&str[inlineStart], inlineEnd-inlineStart, tokens, line, inlineStart-lineStart);

                    inlineStart = inlineEnd + 1;
                } while (inlineStart < lineEnd);
            }
        }

        lineStart = lineEnd + 1;
        line++;
    }
}

static bool cleanup(vector<likely_ast> &atoms)
{
    for (size_t i=0; i<atoms.size(); i++)
        likely_release_ast(atoms[i]);
    atoms.clear();
    return false;
}

likely_ast likely_tokens_from_string(const char *str, bool GFM)
{
    if (!str) return NULL;
    vector<likely_ast> tokens;
    const size_t len = strlen(str);
    if (GFM) tokenizeGFM(str, len, tokens);
    else     tokenize(str, len, tokens, 0, 0);
    return likely_new_list(tokens.data(), tokens.size());
}

static bool shift(likely_const_ast tokens, size_t &offset, vector<likely_ast> &output, int precedence, bool canFail = false);

static int tryReduce(likely_const_ast token, likely_const_ast tokens, size_t &offset, vector<likely_ast> &output, int precedence)
{
    if ((token->type != likely_ast_list) && (precedence < std::numeric_limits<int>::max())) {
        if (!strcmp(token->atom, ".")) {
            if (output.empty())
                return likely_throw(token, "missing operand");
            if (!shift(tokens, offset, output, std::numeric_limits<int>::max()))
                return 0;

            // See if the combined token is a number
            if ((output[output.size()-2]->type != likely_ast_list) &&
                (output[output.size()-1]->type != likely_ast_list) &&
                (isdigit(output[output.size()-2]->atom[0]) || (output[output.size()-2]->atom[0] == '-')) &&
                 isdigit(output[output.size()-1]->atom[0])) {
                // It's a number
                stringstream stream;
                stream << output[output.size()-2]->atom << "." << output[output.size()-1]->atom;
                likely_ast number = likely_new_atom(stream.str().c_str(), stream.str().size());
                number->begin_line   = output[output.size()-2]->begin_line;
                number->begin_column = output[output.size()-2]->begin_column;
                number->end_line     = output[output.size()-1]->end_line;
                number->end_column   = output[output.size()-1]->end_column;
                output.erase(output.end()-2, output.end());
                output.push_back(number);
            } else {
                // It's a composition
                vector<likely_ast> atoms;
                if (output.back()->type == likely_ast_list) {
                    // Inline it
                    for (likely_size i=0; i<output.back()->num_atoms; i++)
                        atoms.push_back(likely_retain_ast(output.back()->atoms[i]));
                    likely_release_ast(output.back());
                } else {
                    atoms.push_back(output.back());
                }
                output.pop_back();

                atoms.insert(atoms.begin() + 1, output.back());
                output.pop_back();

                likely_ast list = likely_new_list(atoms.data(), atoms.size());
                list->begin_line = list->atoms[1]->begin_line;
                list->begin_column = list->atoms[1]->begin_column;
                list->end_line = list->atoms[0]->end_line;
                list->end_column = list->atoms[0]->end_column;
                output.push_back(list);
            }
            return 1;
        }
    }

    return -1;
}

static bool shift(likely_const_ast tokens, size_t &offset, vector<likely_ast> &output, int precedence, bool canFail)
{
    assert(tokens->type == likely_ast_list);
    if (offset >= tokens->num_atoms)
        return likely_throw(tokens->atoms[tokens->num_atoms-1], "unexpected end of expression");
    likely_const_ast token = tokens->atoms[offset++];

    static likely_const_ast comment = likely_new_atom(";", 1);
    if (!likely_ast_compare(token, comment)) {
        const likely_size line = token->begin_line;
        while (token->begin_line == line) {
            if (offset < tokens->num_atoms) token = tokens->atoms[offset++];
            else                            return canFail;
        }
    }

    static likely_const_ast listOpen = likely_new_atom("(", 1);
    static likely_const_ast listClose = likely_new_atom(")", 1);
    static likely_const_ast beginOpen = likely_new_atom("{", 1);
    static likely_const_ast beginClose = likely_new_atom("}", 1);
    if (!likely_ast_compare(token, listOpen) || !likely_ast_compare(token, beginOpen)) {
        vector<likely_ast> atoms;
        likely_const_ast close;
        if (!likely_ast_compare(token, listOpen)) {
            close = listClose;
        } else {
            atoms.push_back(likely_retain_ast(token));
            close = beginClose;
        }

        likely_const_ast end;
        while (true) {
            end = tokens->atoms[offset];
            if (!likely_ast_compare(end, close)) {
                offset++;
                break;
            }
            if (!shift(tokens, offset, atoms, atoms.empty() ? std::numeric_limits<int>::max() : 0))
                return cleanup(atoms);
        }

        if (atoms.size() == 1) {
            output.push_back(atoms[0]);
        } else {
            likely_ast list = likely_new_list(atoms.data(), atoms.size());
            list->begin_line = token->begin_line;
            list->begin_column = token->begin_column;
            list->end_line = end->end_line;
            list->end_column = end->end_column;
            output.push_back(list);
        }
    } else {
        const int result = tryReduce(token, tokens, offset, output, precedence);
        if      (result == 0) return false;
        else if (result == -1) output.push_back(likely_retain_ast(token));
    }

    return true;
}

likely_ast likely_ast_from_tokens(likely_const_ast tokens)
{
    size_t offset = 0;
    vector<likely_ast> expressions;
    while (offset < tokens->num_atoms)
        if (!shift(tokens, offset, expressions, 0, true)) {
            cleanup(expressions);
            return NULL;
        }
    return likely_new_list(expressions.data(), expressions.size());
}

likely_ast likely_ast_from_string(const char *str, bool GFM)
{
    likely_const_ast tokens = likely_tokens_from_string(str, GFM);
    likely_ast ast = likely_ast_from_tokens(tokens);
    likely_release_ast(tokens);
    return ast;
}

static void print(const likely_const_ast ast, stringstream &stream)
{
    if (ast->type == likely_ast_list) {
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

int likely_ast_compare(likely_const_ast a, likely_const_ast b)
{
    if ((a->type == likely_ast_list) == (b->type == likely_ast_list)) {
        if (a->type == likely_ast_list) {
            if (a->num_atoms == b->num_atoms) {
                for (size_t i = 0; i<a->num_atoms; i++)
                    if (int compare = likely_ast_compare(a->atoms[i], b->atoms[i]))
                        return compare;
                return 0;
            } else {
                return (a->num_atoms > b->num_atoms) ? 1 : -1;
            }
        } else {
            return strcmp(a->atom, b->atom);
        }
    } else {
        return (a->type == likely_ast_list) ? 1 : -1;
    }
}

bool likely_ast_contains(likely_const_ast ast, likely_const_ast sub_ast)
{
    if (!likely_ast_compare(ast, sub_ast))
        return true;
    if (ast->type == likely_ast_list)
        for (likely_size i=0; i<ast->num_atoms; i++)
            if (likely_ast_contains(ast->atoms[i], sub_ast))
                return true;
    return false;
}

const char *likely_get_symbol_name(likely_const_ast ast)
{
    while (ast && (ast->type == likely_ast_list) && (ast->num_atoms > 0)) {
        if ((ast->num_atoms > 1) && (ast->atoms[0]->type != likely_ast_list) && !strcmp(ast->atoms[0]->atom, "="))
            ast = ast->atoms[1];
        else
            ast = ast->atoms[0];
    }
    return (ast && (ast->type != likely_ast_list)) ? ast->atom : "";
}

static void default_error_callback(likely_error error, void *)
{
    likely_mat str = likely_error_to_string(error);
    cerr << str->data << endl;
    likely_release(str);
}

static likely_error_callback ErrorCallback = default_error_callback;
static void *ErrorContext = NULL;

void likely_set_error_callback(likely_error_callback callback, void *context)
{
    ErrorCallback = callback;
    ErrorContext = context;
}

bool likely_throw(likely_const_ast where, const char *what)
{
    likely_error error;
    error.where = where;
    error.what = what;
    ErrorCallback(error, ErrorContext);
    return false;
}

likely_mat likely_error_to_string(likely_error error)
{
    stringstream stream;
    stream << error.what << " at: ";
    print(error.where, stream);
    return likely_string(stream.str().c_str());
}
