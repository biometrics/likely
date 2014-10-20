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

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <vector>

#include "likely/frontend.h"

using namespace std;

likely_ast likely_new_atom(const char *atom, uint32_t atom_len)
{
    likely_ast ast = (likely_ast) malloc(sizeof(likely_abstract_syntax_tree) + atom_len + 1);
    if (!ast)
        return NULL;

    const_cast<const char*&>(ast->atom) = reinterpret_cast<const char*>(ast + 1);
    memcpy((void*) ast->atom, atom, atom_len);
    ((char*) ast->atom)[atom_len] = '\0';
    ast->atom_len = atom_len;
    ast->parent = NULL;
    ast->ref_count = 1;
    ast->begin_line = 0;
    ast->begin_column = 0;
    ast->end_line = 0;
    ast->end_column = 0;
    ast->type = likely_ast_atom;
    return ast;
}

likely_ast likely_new_list(const likely_ast *atoms, likely_size num_atoms)
{
    likely_ast ast = (likely_ast) malloc(sizeof(likely_abstract_syntax_tree) + num_atoms * sizeof(likely_ast));
    if (!ast)
        return NULL;

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

likely_ast likely_retain_ast(likely_const_ast ast)
{
    if (!ast) return NULL;
    assert(ast->ref_count > 0);
    const_cast<likely_ast>(ast)->ref_count++;
    return const_cast<likely_ast>(ast);
}

void likely_release_ast(likely_const_ast ast)
{
    if (!ast) return;
    assert(ast->ref_count > 0);
    if (--const_cast<likely_ast>(ast)->ref_count) return;
    if (ast->type == likely_ast_list)
        for (size_t i=0; i<ast->num_atoms; i++)
            likely_release_ast(ast->atoms[i]);
    likely_release_ast(ast->parent);
    free((void*) ast);
}

likely_err likely_new_err(likely_const_err parent, likely_const_ast where, const char *format, ...)
{
    const unsigned MaxErrorLength = 256;
    likely_err err = (likely_err) malloc(sizeof(likely_error) + MaxErrorLength);
    if (!err)
        return NULL;

    err->parent = likely_retain_err(parent);
    err->ref_count = 1;
    err->where = likely_retain_ast(where);
    va_list args;
    va_start(args, format);
    vsnprintf(err->what, MaxErrorLength, format, args);
    return err;
}

likely_err likely_retain_err(likely_const_err err)
{
    if (!err) return NULL;
    assert(err->ref_count > 0);
    const_cast<likely_err>(err)->ref_count++;
    return const_cast<likely_err>(err);
}

void likely_release_err(likely_const_err err)
{
    if (!err) return;
    assert(err->ref_count > 0);
    if (--const_cast<likely_err>(err)->ref_count) return;
    likely_release_ast(err->where);
    likely_release_err(err->parent);
    free((void*) err);
}

static void defaultErrorCallback(likely_err err, void *)
{
    likely_mat str = likely_error_to_string(err);
    cerr << str->data << endl;
    likely_release(str);
}

static likely_error_callback ErrorCallback = defaultErrorCallback;
static void *ErrorContext = NULL;

void likely_set_error_callback(likely_error_callback callback, void *context)
{
    ErrorCallback = callback;
    ErrorContext = context;
}

bool likely_throw(likely_const_ast where, const char *what)
{
    likely_err err = likely_new_err(NULL, where, what);
    ErrorCallback(err, ErrorContext);
    likely_release_err(err);
    return false;
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

likely_mat likely_error_to_string(likely_err err)
{
    stringstream stream;
    stream << err->what << " at: ";
    print(err->where, stream);
    return likely_string(stream.str().c_str());
}

void likely_assert(bool condition, const char *format, ...)
{
    va_list ap;
    if (condition)
        return;

    va_start(ap, format);
    fprintf(stderr, "Likely ");
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");

#ifdef _WIN32
    exit(EXIT_FAILURE); // We prefer not to trigger the Windows crash dialog box
#else // !_WIN32
    abort();
#endif // _WIN32
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
                for (size_t i=lineStart+3; i<lineEnd; i++)
                    if (str[i] > ' ') {
                        skipBlock = true;
                        break;
                    }
                if (!skipBlock)
                    inBlock = true;
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

likely_ast likely_lex(const char *source, likely_source_type type)
{
    if (!source)
        return NULL;

    vector<likely_ast> tokens;
    const size_t len = strlen(source);
    switch (type) {
      case likely_source_lisp:
        tokenize(source, len, tokens, 0, 0);
        break;
      case likely_source_gfm:
        tokenizeGFM(source, len, tokens);
        break;
      default:
        return NULL;
    }

    return likely_new_list(tokens.data(), tokens.size());
}

static bool shift(likely_const_ast tokens, size_t &offset, vector<likely_ast> &output, bool canFail = false);

enum ReductionStatus
{
    ReductionNoOp = -1,
    ReductionFailure = 0,
    ReductionSuccess = 1
};

static ReductionStatus reduceComposition(likely_const_ast tokens, size_t &offset, vector<likely_ast> &output)
{
    if (offset == tokens->num_atoms)
        return ReductionNoOp;

    static likely_const_ast composition = likely_new_atom(".", 1);
    if (!likely_ast_compare(tokens->atoms[offset], composition)) {
        if (output.empty()) {
            likely_throw(tokens->atoms[offset], "missing composition left-hand-side operand");
            return ReductionFailure;
        }
        offset++;
        if (!shift(tokens, offset, output)) {
            likely_throw(tokens->atoms[offset], "missing composition right-hand-side operator/expression");
            return ReductionFailure;
        }

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
            likely_release_ast(output.back()); output.pop_back();
            likely_release_ast(output.back()); output.pop_back();
            output.push_back(number);
        } else {
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
            const likely_size end_line = atoms.back()->end_line;
            const likely_size end_column = atoms.back()->end_column;
            atoms.insert(atoms.begin() + 1, output.back());
            output.pop_back();

            likely_ast list = likely_new_list(atoms.data(), atoms.size());
            list->begin_line = list->atoms[1]->begin_line;
            list->begin_column = list->atoms[1]->begin_column;
            list->end_line = end_line;
            list->end_column = end_column;
            output.push_back(list);
        }

        return reduceComposition(tokens, offset, output) ? ReductionSuccess : ReductionFailure;
    }

    return ReductionNoOp;
}

static ReductionStatus reduce(likely_const_ast tokens, size_t &offset, vector<likely_ast> &output)
{
    assert(tokens->type == likely_ast_list);
    if (offset >= tokens->num_atoms)
        return ReductionNoOp;

    static likely_const_ast infix = likely_new_atom(":", 1);
    if (!likely_ast_compare(tokens->atoms[offset], infix)) {
        if (output.empty()) {
            likely_throw(tokens->atoms[offset], "missing infix left-hand-side operand");
            return ReductionFailure;
        }
        offset++;
        if (!shift(tokens, offset, output)) {
            likely_throw(tokens->atoms[offset], "missing infix operator");
            return ReductionFailure;
        }
        if (!reduce(tokens, offset, output))
            return ReductionFailure;
        if (!shift(tokens, offset, output)) {
            likely_throw(tokens->atoms[offset], "missing infix right-hand-size operand");
            return ReductionFailure;
        }
        if (!reduce(tokens, offset, output))
            return ReductionFailure;

        vector<likely_ast> atoms;
        atoms.push_back(output.back());
        output.pop_back();
        const likely_size end_line = atoms.back()->end_line;
        const likely_size end_column = atoms.back()->end_column;

        atoms.insert(atoms.begin(), output.back());
        output.pop_back();
        atoms.insert(atoms.begin() + 1, output.back());
        output.pop_back();

        likely_ast list = likely_new_list(atoms.data(), atoms.size());
        list->begin_line = list->atoms[1]->begin_line;
        list->begin_column = list->atoms[1]->begin_column;
        list->end_line = end_line;
        list->end_column = end_column;
        output.push_back(list);
    } else {
        const ReductionStatus status = reduceComposition(tokens, offset, output);
        if (status != ReductionSuccess)
            return status;
    }

    return reduce(tokens, offset, output) ? ReductionSuccess : ReductionFailure;
}

// All callers should check this function's return value and likely_throw a
// meaningful error on failure.
static bool shift(likely_const_ast tokens, size_t &offset, vector<likely_ast> &output, bool canFail)
{
    assert(tokens->type == likely_ast_list);
    if (offset >= tokens->num_atoms)
        return false;
    likely_const_ast token = tokens->atoms[offset++];

    static likely_const_ast comment = likely_new_atom(";", 1);
    if (!likely_ast_compare(token, comment)) {
        const likely_size line = token->begin_line;
        while (token->begin_line == line) {
            if (offset < tokens->num_atoms)
                token = tokens->atoms[offset++];
            else
                return canFail;
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

        likely_const_ast end = NULL;
        while (!end) {
            bool success = shift(tokens, offset, atoms);
            if (success) {
                if (!likely_ast_compare(atoms.back(), close)) {
                    end = atoms.back();
                    atoms.pop_back();
                } else {
                    success = success && reduce(tokens, offset, atoms);
                }
            } else {
                stringstream stream;
                stream << "missing list closing token: " << close->atom;
                likely_throw(atoms.back(), stream.str().c_str());
            }
            if (!success)
                return cleanup(atoms);
        }

        likely_ast list = likely_new_list(atoms.data(), atoms.size());
        list->begin_line = token->begin_line;
        list->begin_column = token->begin_column;
        list->end_line = end->end_line;
        list->end_column = end->end_column;
        output.push_back(list);
        likely_release_ast(end);
    } else {
        output.push_back(likely_retain_ast(token));
    }

    return true;
}

likely_ast likely_parse(likely_const_ast tokens)
{
    size_t offset = 0;
    vector<likely_ast> expressions;
    while (offset < tokens->num_atoms)
        if (!shift(tokens, offset, expressions, true) ||
            !reduce(tokens, offset, expressions)) {
            cleanup(expressions);
            return NULL;
        }
    return likely_new_list(expressions.data(), expressions.size());
}

//! [likely_lex_and_parse implementation.]
likely_ast likely_lex_and_parse(const char *source, likely_source_type type)
{
    likely_const_ast tokens = likely_lex(source, type);
    likely_ast ast = likely_parse(tokens);
    likely_release_ast(tokens);
    return ast;
}
//! [likely_lex_and_parse implementation.]

likely_mat likely_ast_to_string(likely_const_ast ast)
{
    stringstream stream;
    print(ast, stream);
    return likely_string(stream.str().c_str());
}

int likely_ast_compare(likely_const_ast a, likely_const_ast b)
{
    if (!a || !b) {
        if (!a && !b) return 0;
        else if (!a)  return -1;
        else /* !b */ return 1;
    }

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

const char *likely_symbol(likely_const_ast ast)
{
    while (ast && (ast->type == likely_ast_list) && (ast->num_atoms > 0)) {
        if ((ast->num_atoms > 1) && (ast->atoms[0]->type != likely_ast_list) && !strcmp(ast->atoms[0]->atom, "="))
            ast = ast->atoms[1];
        else
            ast = ast->atoms[0];
    }
    return (ast && (ast->type != likely_ast_list)) ? ast->atom : "";
}

likely_mat likely_type_to_string(likely_size type)
{
    stringstream typeStream;
    typeStream << ((type & likely_matrix_floating) ? "f" : ((type & likely_matrix_signed) ? "i" : "u"));
    typeStream << (type & likely_matrix_depth);
    if (type & likely_matrix_array)         typeStream << "A";
    if (type & likely_matrix_saturated)     typeStream << "S";
    if (type & likely_matrix_multi_channel) typeStream << "C";
    if (type & likely_matrix_multi_column)  typeStream << "X";
    if (type & likely_matrix_multi_row)     typeStream << "Y";
    if (type & likely_matrix_multi_frame)   typeStream << "T";
    return likely_string(typeStream.str().c_str());
}

likely_mat likely_type_field_to_string(likely_size type)
{
    if (type == likely_matrix_void           ) return likely_string("void");
    if (type == likely_matrix_depth          ) return likely_string("depth");
    if (type == likely_matrix_floating       ) return likely_string("floating");
    if (type == likely_matrix_array          ) return likely_string("array");
    if (type == likely_matrix_signed         ) return likely_string("signed");
    if (type == likely_matrix_saturated      ) return likely_string("saturated");
    if (type == likely_matrix_element        ) return likely_string("element");
    if (type == likely_matrix_multi_channel  ) return likely_string("multi_channel");
    if (type == likely_matrix_multi_column   ) return likely_string("multi_column");
    if (type == likely_matrix_multi_row      ) return likely_string("multi_row");
    if (type == likely_matrix_multi_frame    ) return likely_string("multi_frame");
    if (type == likely_matrix_multi_dimension) return likely_string("multi_dimension");
    return likely_type_to_string(type);
}

likely_matrix_type likely_type_from_string(const char *str, bool *ok)
{
    size_t length;
    uint32_t type;
    char *remainder;

    if (!str)
        goto error;

    if (ok)
        *ok = true;

    // Special cases
    if (!strcmp(str, "void"           )) return likely_matrix_void;
    if (!strcmp(str, "depth"          )) return likely_matrix_depth;
    if (!strcmp(str, "floating"       )) return likely_matrix_floating;
    if (!strcmp(str, "array"          )) return likely_matrix_array;
    if (!strcmp(str, "signed"         )) return likely_matrix_signed;
    if (!strcmp(str, "saturated"      )) return likely_matrix_saturated;
    if (!strcmp(str, "element"        )) return likely_matrix_element;
    if (!strcmp(str, "multi_channel"  )) return likely_matrix_multi_channel;
    if (!strcmp(str, "multi_column"   )) return likely_matrix_multi_column;
    if (!strcmp(str, "multi_row"      )) return likely_matrix_multi_row;
    if (!strcmp(str, "multi_frame"    )) return likely_matrix_multi_frame;
    if (!strcmp(str, "multi_dimension")) return likely_matrix_multi_dimension;
    if (!strcmp(str, "string"         )) return likely_matrix_string;
    if (!strcmp(str, "native"         )) return likely_matrix_native;

    // General case
    length = strlen(str);
    if (length == 0)
        goto error;

    if      (str[0] == 'f') type = likely_matrix_floating;
    else if (str[0] == 'i') type = likely_matrix_signed;
    else if (str[0] == 'u') type = likely_matrix_void;
    else                    goto error;

    type += (int)strtol(str+1, &remainder, 10);

    while (*remainder) {
        if      (*remainder == 'A') type |= likely_matrix_array;
        else if (*remainder == 'S') type |= likely_matrix_saturated;
        else if (*remainder == 'C') type |= likely_matrix_multi_channel;
        else if (*remainder == 'X') type |= likely_matrix_multi_column;
        else if (*remainder == 'Y') type |= likely_matrix_multi_row;
        else if (*remainder == 'T') type |= likely_matrix_multi_frame;
        else                        goto error;
        remainder++;
    }

    return (likely_matrix_type) type;

error:
    if (ok)
        *ok = false;
    return likely_matrix_void;
}

likely_size likely_type_from_value(double value)
{
    if      (int32_t(value) == value) return likely_matrix_i32;
    else if (int64_t(value) == value) return likely_matrix_i64;
    else if (float(value)   == value) return likely_matrix_f32;
    else                              return likely_matrix_f64;
}

likely_size likely_type_from_types(likely_size lhs, likely_size rhs)
{
    likely_size result = (lhs | rhs) & ~likely_matrix_depth;
    result |= max(lhs & likely_matrix_depth, rhs & likely_matrix_depth);
    if (result & likely_matrix_floating)
        result &= ~likely_matrix_signed;
    return result;
}
