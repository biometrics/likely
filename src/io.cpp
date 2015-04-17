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

#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#  define NOMINMAX
#endif

#include <cstdarg>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <future>
#include <string>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <opencv2/highgui/highgui.hpp>

#include "likely/backend.h"
#include "likely/io.h"
#include "likely/opencv.hpp"

using namespace llvm;
using namespace std;

//! [likely_guess_file_type implementation.]
likely_file_type likely_guess_file_type(const char *file_name)
{
    if (!file_name)
        return likely_file_void;
    const char *extension = strrchr(file_name, '.');
    if (!extension)
        return likely_file_directory;
    extension++; // remove the leading '.'
    if (!strcmp(extension, "bin" )) return likely_file_binary;
    if (!strcmp(extension, "lm"  )) return likely_file_matrix;
    if (!strcmp(extension, "txt" )) return likely_file_text;
    if (!strcmp(extension, "lisp")) return likely_file_lisp;
    if (!strcmp(extension, "md"  )) return likely_file_gfm;
    if (!strcmp(extension, "tex" )) return likely_file_tex;
    if (!strcmp(extension, "ll"  )) return likely_file_ir;
    if (!strcmp(extension, "bc"  )) return likely_file_bitcode;
    if (!strcmp(extension, "o"   )) return likely_file_object;
    if (!strcmp(extension, "s"   )) return likely_file_assembly;

    return likely_file_media;
}
//! [likely_guess_file_type implementation.]

//! [likely_file_type_from_string implementation.]
likely_file_type likely_file_type_from_string(const char *str, bool *ok)
{
    if (!str)
        goto error;

    if (ok)
        *ok = true;

    if (!strcmp(str, "void"     )) return likely_file_void;
    if (!strcmp(str, "directory")) return likely_file_directory;
    if (!strcmp(str, "binary"   )) return likely_file_binary;
    if (!strcmp(str, "media"    )) return likely_file_media;
    if (!strcmp(str, "matrix"   )) return likely_file_matrix;
    if (!strcmp(str, "text"     )) return likely_file_text;
    if (!strcmp(str, "lisp"     )) return likely_file_lisp;
    if (!strcmp(str, "gfm"      )) return likely_file_gfm;
    if (!strcmp(str, "tex"      )) return likely_file_tex;
    if (!strcmp(str, "ir"       )) return likely_file_ir;
    if (!strcmp(str, "bitcode"  )) return likely_file_bitcode;
    if (!strcmp(str, "object"   )) return likely_file_object;
    if (!strcmp(str, "assembly" )) return likely_file_assembly;
    if (!strcmp(str, "guess"    )) return likely_file_guess;

error:
    if (ok)
        *ok = false;
    return likely_file_void;
}
//! [likely_file_type_from_string implementation.]

static likely_mat readAsync(const string &fileName, likely_type type)
{
    return likely_read(fileName.c_str(), likely_file_guess, type);
}

likely_mat likely_read(const char *file_name, likely_file_type file_type, likely_type type)
{
    // Interpret ~ as $HOME
    string fileName = file_name;
    if (fileName[0] == '~') {
        static string HOME = getenv("HOME");
        fileName = HOME + fileName.substr(1);
    }

    if (file_type == likely_file_guess)
        file_type = likely_guess_file_type(file_name);

    likely_mat result = NULL;
    if (file_type == likely_file_directory) {
        error_code ec;
        vector<future<likely_mat>> futures;
        for (sys::fs::recursive_directory_iterator i(fileName, ec), e; i != e; i.increment(ec))
            if (sys::fs::is_regular_file(i->path()) && sys::path::has_stem(i->path()))
                futures.push_back(async(readAsync, i->path(), type & ~likely_multi_frame));

        // Combine into one matrix with multiple frames
        likely_matrix firstHeader;
        size_t step = 0;
        bool valid = true;
        for (size_t i=0; i<futures.size(); i++) {
            likely_const_mat image = futures[i].get();
            if ((i == 0) && image) {
                firstHeader = *image;
                step = likely_bytes(&firstHeader);
                likely_type type = firstHeader.type;
                if (futures.size() > 1)
                    type |= likely_multi_frame;
                result = likely_new(type, firstHeader.channels, firstHeader.columns, firstHeader.rows, uint32_t(firstHeader.frames * futures.size()), NULL);
            }

            valid = valid
                    && image
                    && (image->type     == firstHeader.type)
                    && (image->channels == firstHeader.channels)
                    && (image->columns  == firstHeader.columns)
                    && (image->rows     == firstHeader.rows)
                    && (image->frames   == firstHeader.frames);

            if (valid) {
                memcpy(result->data + i*step, image->data, step);
            } else if (result) {
                assert(!valid);
                free(result);
                result = NULL;
            }
            likely_release_mat(image);
        }
    } else {
        FILE *fp;
        { // we lock to ensure that fopen + perror is atomic
            static mutex m;
            unique_lock<mutex> lock(m);
            fp = fopen(fileName.c_str(), "rb");
            if (!fp)
                perror(NULL);
        }
        likely_ensure(fp != NULL, "failed to open: %s", file_name);
        if (fp) {
            fseek(fp, 0, SEEK_END);
            const size_t size = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            if (file_type & likely_file_matrix) {
                likely_matrix header;
                if (fread(&header, sizeof(likely_matrix), 1, fp)) {
                    const size_t bytes = likely_bytes(&header);
                    likely_ensure(sizeof(likely_matrix) + bytes == size, "mismatch between matrix header and file size");
                    result = likely_new(header.type, header.channels, header.columns, header.rows, header.frames, NULL);
                    const bool success = (fread(result->data, bytes, 1, fp) == 1);
                    assert(success);
                    if (!success) {
                        likely_release_mat(result);
                        result = NULL;
                    }
                }
            } else {
                likely_mat buffer = likely_new(likely_u8 | likely_multi_channel, uint32_t(size + ((file_type & likely_file_text) ? 1 : 0)), 1, 1, 1, NULL);
                const bool success = (fread(buffer->data, 1, size, fp) == size);
                likely_ensure(success, "failed to read: %s", file_name);
                if (success) {
                    if (file_type & likely_file_binary) {
                        result = likely_retain_mat(buffer);
                    } else if (file_type & likely_file_text) {
                        buffer->data[size] = 0;
                        buffer->type |= likely_signed;
                        result = likely_retain_mat(buffer);
                    } else if (file_type == likely_file_media) {
                        swap(buffer->channels, buffer->columns);
                        result = likely_decode(buffer, type);
                    }
                }
                likely_release_mat(buffer);
            }
            fclose(fp);
        }
    }
    likely_ensure(result != NULL, "failed to process: %s", file_name);
    if (result && (type != likely_void) && (result->type != type)) {
        const likely_const_mat expected = likely_type_to_string(type);
        const likely_const_mat actual = likely_type_to_string(result->type);
        likely_ensure(false, "read type mismatch for: %s, expected: %s but got: %s", file_name, expected->data, actual->data);
        likely_release_mat(actual);
        likely_release_mat(expected);
        likely_release_mat(result);
        result = NULL;
    }
    return result;
}

bool likely_write(likely_const_mat image, const char *file_name)
{
    const likely_file_type fileType = likely_guess_file_type(file_name);
    if (fileType == likely_file_media) {
        try {
            cv::imwrite(file_name, likelyToOpenCVMat(image));
        } catch (...) {
            return false;
        }
    } else {
        if (FILE *const fp = fopen(file_name, "wb")) {
            if (fileType == likely_file_matrix) {
                const uint32_t ref_count = 1;
                fwrite(&ref_count, sizeof(uint32_t), 1, fp);
                fwrite(reinterpret_cast<uint32_t const*>(image)+1, sizeof(likely_matrix)-sizeof(uint32_t)+likely_bytes(image), 1, fp);
            } else {
                const size_t size = (fileType & likely_file_text) ? strlen(image->data)
                                                                  : likely_bytes(image);
                fwrite(image->data, size, 1, fp);
            }
            fclose(fp);
        } else {
            return false;
        }
    }
    return true;
}

likely_mat likely_decode(likely_const_mat buffer, likely_type type)
{
    likely_mat result = NULL;
    try {
        result = likelyFromOpenCVMat(cv::imdecode(likelyToOpenCVMat(buffer),
                                                  (type & likely_multi_channel) ? CV_LOAD_IMAGE_COLOR
                                                                                : CV_LOAD_IMAGE_GRAYSCALE));
    } catch (...) {}

    likely_ensure(result != NULL, "decode failure");
    if (result && (result->type != type)) {
        likely_release_mat(result);
        result = NULL;
        likely_ensure(false, "decode type mismatch");
    }
    return result;
}

likely_mat likely_encode(likely_const_mat image, const char *extension)
{
    vector<uchar> buf;
    try {
        cv::imencode(string(".") + extension, likelyToOpenCVMat(image), buf);
    } catch (...) {
        return NULL;
    }
    return likelyFromOpenCVMat(cv::Mat(buf));
}

likely_mat likely_to_string(likely_const_mat mat)
{
    return likely_to_string_n(&mat, 1);
}

static void printElement(stringstream &buffer, double value, likely_type type)
{
    if (type & likely_floating) {
        bool looksLikeAnInteger;
        {
            stringstream tmpStream;
            tmpStream << value;
            const string tmpString = tmpStream.str();
            char *p;
            strtoll(tmpString.c_str(), &p, 10);
            looksLikeAnInteger = (*p == 0);
        }

#ifdef _MSC_VER
    const unsigned int originalExponentFormat = _set_output_format(_TWO_DIGIT_EXPONENT); // for cross-platform consistency
#endif // _MSC_VER

        buffer << value;

#ifdef _MSC_VER
    _set_output_format(originalExponentFormat); // restore original format
#endif // _MSC_VER

        if (looksLikeAnInteger)
            buffer << ".0"; // Print a significant figure to distinguish floating-point values from integers
    } else {
        buffer << int64_t(value);
    }
}

likely_mat likely_to_string_n(likely_const_mat *mats, size_t n)
{
    stringstream buffer;
    for (size_t i=0; i<n; i++) {
        likely_const_mat m = mats[i];
        if (!m) {
            // skip it
        } else if (likely_is_string(m)) {
            buffer << m->data;
        } else if (!(m->type & likely_multi_dimension)) {
            printElement(buffer, likely_get_element(m, 0, 0, 0, 0), m->type);
        } else {
            buffer << "(";
            likely_mat str = likely_type_to_string(m->type);
            buffer << str->data << " ";
            likely_release_mat(str);

            buffer << (m->frames > 1 ? "(" : "");
            for (uint32_t t=0; t<m->frames; t++) {
                buffer << (m->rows > 1 ? "(" : "");
                for (uint32_t y=0; y<m->rows; y++) {
                    buffer << (m->columns > 1 ? "(" : "");
                    for (uint32_t x=0; x<m->columns; x++) {
                        buffer << (m->channels > 1 ? "(" : "");
                        for (uint32_t c=0; c<m->channels; c++) {
                            printElement(buffer, likely_get_element(m, c, x, y, t), m->type);
                            if (c != m->channels-1)
                                buffer << " ";
                        }
                        buffer << (m->channels > 1 ? ")" : "");
                        if (x != m->columns-1)
                            buffer << " ";
                    }
                    buffer << (m->columns > 1 ? ")" : "");
                    if (y != m->rows-1)
                        buffer << "\n";
                }
                buffer << (m->rows > 1 ? ")" : "");
                if (t != m->frames-1)
                    buffer << "\n\n";
            }
            buffer << ((m->frames > 1) ? ")" : "");

            const bool frames   =            (m->frames   > 1);
            const bool rows     = frames  || (m->rows     > 1);
            const bool columns  = rows    || (m->columns  > 1);
            const bool channels = columns || (m->channels > 1);
            if (channels) buffer << " (" << m->channels;
            if (columns ) buffer << " "  << m->columns;
            if (rows    ) buffer << " "  << m->rows;
            if (frames  ) buffer << " "  << m->frames;
            if (channels) buffer << ")";
            buffer << ")";
        }
    }

    const string str = buffer.str();
    return str.empty() ? NULL : likely_string(str.c_str());
}

likely_mat likely_to_string_va(likely_const_mat mat, ...)
{
    va_list ap;
    va_start(ap, mat);
    vector<likely_const_mat> mv;
    while (mat) {
        mv.push_back(mat);
        mat = va_arg(ap, likely_const_mat);
    }
    return likely_to_string_n(mv.data(), mv.size());
}

likely_mat likely_render(likely_const_mat mat, double *min_, double *max_)
{
    if (!mat)
        return NULL;

    // Special case, return the original image
    if (((mat->type & likely_element) == likely_u8) && (mat->channels == 3)) {
        if (min_) *min_ = 0;
        if (max_) *max_ = 255;
        return likely_retain_mat(mat);
    }

    double min, max, range;
    static likely_const_env minMaxEnv = NULL;
    static void *minMax = NULL;
    if (minMax == NULL) {
        const likely_env parent = likely_standard(likely_default_settings(likely_file_void, false), NULL, likely_file_void);
        const char *const src = "-likely-minmax :=                   \n"
                                "  (src min max) :->                 \n"
                                "  {                                 \n"
                                "    (? (== (& src.type element) u8) \n"
                                "      {                             \n"
                                "        min :<- 0                   \n"
                                "        max :<- 255                 \n"
                                "      } {                           \n"
                                "        min :<- src.min-element     \n"
                                "        max :<- src.max-element     \n"
                                "      })                            \n"
                                "  }                                 \n"
                                "(extern void \"_likely_minmax\" (multi-dimension double.pointer double.pointer) -likely-minmax) \n";
        minMaxEnv = likely_lex_parse_and_eval(src, likely_file_lisp, parent);
        minMax = likely_function(minMaxEnv->expr);
        assert(minMax);
        likely_release_env(parent);
    }
    reinterpret_cast<void (*)(likely_const_mat, double*, double*)>(minMax)(mat, &min, &max);
    range = (max - min) / 255;

    static likely_const_env env = NULL;
    static void *normalize = NULL;
    if (normalize == NULL) {
        const likely_env parent = likely_standard(likely_default_settings(likely_file_void, false), NULL, likely_file_void);
        const char *const src = "-likely-normalize :=\n"
                                "  (src a b) :->\n"
                                "  {\n"
                                "    dst := (new u8CXY 3 src.columns src.rows 1 null)\n"
                                "    (dst src a b) :=>\n"
                                "      dst :<- (* src a).(+ b)\n"
                                "  }\n"
                                "(extern multi-dimension \"_likely_normalize\" (multi-dimension double double) -likely-normalize)";
        env = likely_lex_parse_and_eval(src, likely_file_lisp, parent);
        normalize = likely_function(env->expr);
        assert(normalize);
        likely_release_env(parent);
    }

    const likely_mat n = reinterpret_cast<likely_mat (*)(likely_const_mat, double, double)>(normalize)(mat, 1/range, -min/range);
    if (min_) *min_ = min;
    if (max_) *max_ = max;
    return n;
}

void likely_show(likely_const_mat image, const char *title)
{
    if (!image || !title)
        return;
    cv::imshow(title, likelyToOpenCVMat(image));
    cv::waitKey();
}

void likely_ensure(bool condition, const char *format, ...)
{
    if (condition)
        return;

    va_list ap;
    va_start(ap, format);
    fprintf(stderr, "Likely Error - ");
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    assert(false); // Abort in debug mode, exit in release mode

    exit(EXIT_FAILURE);
}

void likely_ensure_approximate(likely_const_mat a, likely_const_mat b, float error_threshold)
{
    likely_ensure(a->channels == b->channels, "channels: %d and: %d differ!", a->channels, b->channels);
    likely_ensure(a->columns  == b->columns , "columns: %d and: %d differ!" , a->columns , b->columns);
    likely_ensure(a->rows     == b->rows    , "rows: %d and: %d differ!"    , a->rows    , b->rows);
    likely_ensure(a->frames   == b->frames  , "frames: %d and: %d differ!"  , a->frames  , b->frames);
    double delta = 0;
    double sum = 0;
    for (uint32_t t=0; t<a->frames; t++)
        for (uint32_t y=0; y<a->rows; y++)
            for (uint32_t x=0; x<a->columns; x++)
                for (uint32_t c=0; c<a->channels; c++) {
                    const double A = likely_get_element(a, c, x, y, t);
                    const double B = likely_get_element(b, c, x, y, t);
                    delta += fabs(A - B);
                    sum += max(A, B);
                }
    const float relativeError = float(delta / sum);
    likely_ensure(relativeError < error_threshold, "average delta: %g%% greater than threshold: %g%%", relativeError, error_threshold);
}
