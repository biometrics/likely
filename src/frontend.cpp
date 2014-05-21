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

likely_ast likely_new_atom(const char *str)
{
    return likely_new_atom_at(str, 0, strlen(str));
}

likely_ast likely_new_atom_at(const char *str, size_t begin, size_t end)
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

likely_ast likely_new_list(const likely_const_ast *atoms, size_t num_atoms)
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
        while ((i < len) && (inString || ((str[i] > ignored) && (str[i] != '(')
                                                             && (str[i] != ')')
                                                             && (str[i] != '.')
                                                             && (str[i] != ':')
                                                             && (str[i] != ';')))) {
            if      (str[i] == '"')  inString = !inString;
            else if (str[i] == '\\') i++;
            i++;
        }
        if (i == begin)
            i++;

        tokens.push_back(likely_new_atom_at(str, begin, i));
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

static bool cleanup(vector<likely_const_ast> &atoms)
{
    for (size_t i=0; i<atoms.size(); i++)
        likely_release_ast(atoms[i]);
    atoms.clear();
    return false;
}

likely_ast likely_tokens_from_string(const char *str, bool GFM)
{
    if (!str) return NULL;
    vector<likely_const_ast> tokens;
    const size_t len = strlen(str);
    if (GFM) tokenizeGFM(str, len, tokens);
    else     tokenize(str, len, tokens);
    return likely_new_list(tokens.data(), tokens.size());
}

struct Operator
{
    int precedence;
    size_t rightHandAtoms;
    Operator(int precedence, size_t rightHandAtoms)
        : precedence(precedence), rightHandAtoms(rightHandAtoms) {}
};

// Construct on first use idiom avoids static initialization order fiasco
static map<string, Operator> &ops()
{
    static map<string, Operator> Ops;
    return Ops;
}

void likely_insert_operator(const char *symbol, int precedence, int right_hand_atoms)
{
    ops().insert(pair<string,Operator>(symbol, Operator(precedence, right_hand_atoms)));
}

static bool shift(likely_const_ast tokens, size_t &offset, vector<likely_const_ast> &output, int precedence = 0);

static int tryReduce(likely_const_ast token, likely_const_ast tokens, size_t &offset, vector<likely_const_ast> &output, int precedence)
{
    if (!token->is_list && (precedence < std::numeric_limits<int>::max())) {
        if (!strcmp(token->atom, ".")) {
            if (output.empty())
                return likely_throw(token, "missing operand");
            if (!shift(tokens, offset, output, std::numeric_limits<int>::max()))
                return 0;

            // See if the combined token is a number
            if (!output[output.size()-2]->is_list &&
                !output[output.size()-1]->is_list &&
                (isdigit(output[output.size()-2]->atom[0]) || (output[output.size()-2]->atom[0] == '-'))  &&
                isdigit(output[output.size()-1]->atom[0])) {
                // It's a number
                stringstream stream;
                stream << output[output.size()-2]->atom << "." << output[output.size()-1]->atom;
                likely_ast number = likely_new_atom(stream.str().c_str());
                number->begin = output[output.size()-2]->begin;
                number->end   = output[output.size()-1]->end;
                output.erase(output.end()-2, output.end());
                output.push_back(number);
                return 1;
            }

            // It's a composition
            vector<likely_const_ast> atoms;
            atoms.push_back(output.back()); output.pop_back();
            atoms.push_back(output.back()); output.pop_back();
            output.push_back(likely_new_list(atoms.data(), atoms.size()));
            return 1;
        }

        const auto &op = ops().find(token->atom);
        if ((op != ops().end() && (op->second.precedence > precedence)) || !strcmp(token->atom, "[")) {
            if (output.empty())
                return likely_throw(token, "missing left-hand operand");
            const size_t before = output.size();

            if (!strcmp(token->atom, "[")) {
                static likely_const_ast rightBracket = likely_new_atom("]");
                do {
                    if (!shift(tokens, offset, output, op->second.precedence))
                        return 0;
                } while (likely_ast_compare(output.back(), rightBracket));
                output.pop_back();
            } else {
                for (size_t i=0; i<op->second.rightHandAtoms; i++)
                    if (!shift(tokens, offset, output, op->second.precedence))
                        return 0;
            }
            size_t atoms = 1 + output.size() - before;
            output.insert(output.end()-atoms++, likely_retain_ast(token));
            output.push_back(likely_new_list(&output[output.size()-atoms], atoms));
            output.erase(output.end()-atoms-1, output.end()-1);
            return 1;
        }
    }

    return -1;
}

static bool shift(likely_const_ast tokens, size_t &offset, vector<likely_const_ast> &output, int precedence)
{
    assert(tokens->is_list);
    if (offset >= tokens->num_atoms)
        return likely_throw(tokens->atoms[tokens->num_atoms-1], "unexpected end of expression");

    likely_const_ast token = tokens->atoms[offset++];
    if (!token->is_list && (!strcmp(token->atom, "(") || !strcmp(token->atom, "{"))) {
        vector<likely_const_ast> atoms;
        const char *close;
        if (!strcmp(token->atom, "(")) {
            close = ")";
        } else {
            atoms.push_back(likely_retain_ast(token));
            close = "}";
        }
        likely_const_ast end;
        while (true) {
            end = tokens->atoms[offset];
            if (!end->is_list && !strcmp(end->atom, close)) {
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
            list->begin = token->begin;
            list->end = end->end;
            output.push_back(list);
        }
    } else {
        const int result = tryReduce(token, tokens, offset, output, precedence);
        if      (result == 0) return false;
        else if (result == -1) output.push_back(likely_retain_ast(token));
    }

    // Conventional look ahead
    while (offset < tokens->num_atoms) {
        const int result = tryReduce(tokens->atoms[offset++], tokens, offset, output, precedence);
        if (result == -1) {
            offset--;
            break;
        } else if (result == 0) {
            return false;
        }
    }

    // Special case additional shift(s)
    if (precedence < std::numeric_limits<int>::max())
        while ((offset < tokens->num_atoms) && !tokens->atoms[offset]->is_list && !strcmp(tokens->atoms[offset]->atom, ":"))
            if (!shift(tokens, ++offset, output, 0))
                return false;

    return true;
}

likely_ast likely_ast_from_tokens(likely_const_ast tokens)
{
    size_t offset = 0;
    vector<likely_const_ast> expressions;
    while (offset < tokens->num_atoms)
        if (!shift(tokens, offset, expressions)) {
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

int likely_ast_compare(likely_const_ast a, likely_const_ast b)
{
    if (a->is_list == b->is_list) {
        if (a->is_list) {
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
        return a->is_list ? 1 : -1;
    }
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
