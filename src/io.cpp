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
#include <iostream>
#include <future>
#include <string>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MD5.h>
#include <opencv2/highgui/highgui.hpp>

#include "likely/backend.h"
#include "likely/io.h"
#include "likely/opencv.hpp"

using namespace llvm;
using namespace std;

//! [likely_guess_file_type implementation.]
likely_file_type likely_guess_file_type(const char *file_name)
{
    const char *extension = strrchr(file_name, '.');
    if (!extension)
        return likely_file_directory;
    extension++; // remove the leading '.'
    if      (!strcmp(extension, "bin" )) return likely_file_binary;
    else if (!strcmp(extension, "mat" )) return likely_file_matrix;
    else if (!strcmp(extension, "txt" )) return likely_file_text;
    else if (!strcmp(extension, "lisp")) return likely_file_lisp;
    else if (!strcmp(extension, "md"  )) return likely_file_gfm;
    else                                 return likely_file_media;
}
//! [likely_guess_file_type implementation.]

static likely_mat readAsync(const string &fileName)
{
    return likely_read(fileName.c_str(), likely_guess_file_type(fileName.c_str()));
}

likely_mat likely_read(const char *file_name, likely_file_type type)
{
    // Interpret ~ as $HOME
    string fileName = file_name;
    if (fileName[0] == '~') {
        static string HOME = getenv("HOME");
        fileName = HOME + fileName.substr(1);
    }

    likely_mat result = NULL;
    if (type == likely_file_directory) {
        error_code ec;
        vector<future<likely_mat>> futures;
        for (sys::fs::recursive_directory_iterator i(fileName, ec), e; i != e; i.increment(ec))
            if (sys::fs::is_regular_file(i->path()))
                futures.push_back(async(readAsync, i->path()));

        // Combine into one matrix with multiple frames
        likely_matrix firstHeader;
        size_t step = 0;
        bool valid = true;
        for (size_t i=0; i<futures.size(); i++) {
            likely_const_mat image = futures[i].get();
            if ((i == 0) && image) {
                firstHeader = *image;
                step = likely_bytes(&firstHeader);
                result = likely_new(firstHeader.type, firstHeader.channels, firstHeader.columns, firstHeader.rows, uint32_t(firstHeader.frames * futures.size()), NULL);
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
        if (FILE *fp = fopen(fileName.c_str(), "rb")) {
            fseek(fp, 0, SEEK_END);
            const size_t size = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            if (type & likely_file_matrix) {
                likely_matrix header;
                if (fread(&header, sizeof(likely_matrix), 1, fp)) {
                    const size_t bytes = likely_bytes(&header);
                    if (sizeof(likely_matrix) + bytes == size) {
                        result = likely_new(header.type, header.channels, header.columns, header.rows, header.frames, NULL);
                        const bool success = (fread(result->data, bytes, 1, fp) == 1);
                        assert(success);
                        if (!success) {
                            likely_release_mat(result);
                            result = NULL;
                        }
                    }
                }
            } else {
                likely_mat buffer = likely_new(likely_matrix_u8, uint32_t(size + ((type & likely_file_text) ? 1 : 0)), 1, 1, 1, NULL);
                const bool success = (fread(buffer->data, 1, size, fp) == size);
                if (success) {
                    if (type & likely_file_binary) {
                        result = likely_retain_mat(buffer);
                    } else if (type & likely_file_text) {
                        buffer->data[size] = 0;
                        buffer->type = likely_matrix_string;
                        result = likely_retain_mat(buffer);
                    } else if (type == likely_file_media) {
                        swap(buffer->channels, buffer->columns);
                        result = likely_decode(buffer);
                    }
                }
                likely_release_mat(buffer);
            }
            fclose(fp);
        }
    }
    return result;
}

likely_mat likely_write(likely_const_mat image, const char *file_name)
{
    const size_t len = strlen(file_name);
    if ((len < 3) || strcmp(&file_name[len-3], ".lm")) {
        try {
            cv::imwrite(file_name, likelyToOpenCVMat(image));
        } catch (...) {
            return NULL;
        }
    } else {
        if (FILE *fp = fopen(file_name, "wb")) {
            const uint32_t ref_count = 1;
            fwrite(&ref_count, sizeof(uint32_t), 1, fp);
            fwrite(reinterpret_cast<uint32_t const*>(image)+1, likely_bytes(image)-sizeof(uint32_t), 1, fp);
            fclose(fp);
        } else {
            return NULL;
        }
    }
    return const_cast<likely_mat>(image);
}

likely_mat likely_decode(likely_const_mat buffer)
{
    try {
        return likelyFromOpenCVMat(cv::imdecode(likelyToOpenCVMat(buffer), CV_LOAD_IMAGE_UNCHANGED));
    } catch (...) {}

    return NULL;
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

likely_mat likely_to_string_n(likely_const_mat *mats, size_t n)
{
    stringstream buffer;
    for (size_t i=0; i<n; i++) {
        likely_const_mat m = mats[i];
        if (!m) {
            // skip it
        } else if (likely_is_string(m)) {
            buffer << m->data;
        } else if (!(m->type & likely_matrix_multi_dimension)) {
            buffer << likely_element(m, 0, 0, 0, 0);
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
                            buffer << likely_element(m, c, x, y, t);
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

    return likely_string(buffer.str().c_str());
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

    double min, max, range;
    if ((mat->type & likely_matrix_element) != likely_matrix_u8) {
        min = numeric_limits<double>::max();
        max = -numeric_limits<double>::max();
        for (uint32_t t=0; t<mat->frames; t++) {
            for (uint32_t y=0; y<mat->rows; y++) {
                for (uint32_t x=0; x<mat->columns; x++) {
                    for (uint32_t c=0; c<mat->channels; c++) {
                        const double value = likely_element(mat, c, x, y, t);
                        min = ::min(min, value);
                        max = ::max(max, value);
                    }
                }
            }
        }
        range = (max - min) / 255.0;
    } else {
        min = 0;
        max = 255;
        range = 1;
        // Special case, return the original image
        if (mat->channels == 3) {
            if (min_) *min_ = min;
            if (max_) *max_ = max;
            return likely_retain_mat(mat);
        }
    }

    static likely_const_env env = NULL;
    static void *normalize = NULL;
    if (normalize == NULL) {
        likely_const_ast ast = likely_lex_and_parse("(img min range) :-> { dst := (new u8 3 img.columns img.rows) (dst img min range) :=> (<- dst (- img min).(/ range).u8) }", likely_file_lisp);
        likely_env parent = likely_standard(NULL);
        env = likely_eval(ast->atoms[0], parent);
        assert(env->expr);
        normalize = likely_compile(env->expr, NULL, 0);
        assert(normalize);
        likely_release_env(parent);
        likely_release_ast(ast);
    }

    likely_const_mat min_val = likely_scalar(likely_matrix_f32, &min, 1);
    likely_const_mat range_val = likely_scalar(likely_matrix_f32, &range, 1);
    likely_mat n = reinterpret_cast<likely_mat (*)(likely_const_mat, likely_const_mat, likely_const_mat)>(normalize)(mat, min_val, range_val);
    likely_release_mat(min_val);
    likely_release_mat(range_val);

    if (min_) *min_ = min;
    if (max_) *max_ = max;
    return n;
}

void likely_show(likely_const_mat mat, const char *title)
{
    likely_mat rendered = likely_render(mat, NULL, NULL);
    cv::imshow(title, likelyToOpenCVMat(rendered));
    cv::waitKey();
    likely_release_mat(rendered);
}

likely_mat likely_md5(likely_const_mat mat)
{
    MD5 md5;
    md5.update(ArrayRef<uint8_t>(reinterpret_cast<const uint8_t*>(mat->data), likely_bytes(mat)));
    MD5::MD5Result md5Result;
    md5.final(md5Result);
    return likely_new(likely_matrix_u8, 16, 1, 1, 1, md5Result);
}
