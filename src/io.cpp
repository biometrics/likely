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
#include <opencv2/highgui/highgui.hpp>

#ifdef _WIN32
#include "dirent_windows.h"
#else // !_WIN32
#include <dirent.h>
#endif // _WIN32

#include "likely/backend.h"
#include "likely/io.h"
#include "likely/opencv.hpp"

using namespace std;

static likely_mat takeAndInterpret(likely_mat buffer, size_t type)
{
    likely_mat result = NULL;
    if (!result && (type & likely_file_decoded)) {
        if (likely_bytes(buffer) >= sizeof(likely_matrix)) {
            likely_mat header = (likely_mat) buffer->data;
            if (sizeof(likely_matrix) + likely_bytes(header) == likely_bytes(buffer))
                result = likely_new(header->type, header->channels, header->columns, header->rows, header->frames, header->data);
        }
    }

    if (!result && (type & likely_file_encoded))
        result = likely_decode(buffer);

    if (!result && (type & likely_file_text)) {
        const size_t bytes = likely_bytes(buffer);
        buffer->data[bytes-1] = 0;
        buffer->channels = uint32_t(bytes);
        buffer->columns = buffer->rows = buffer->frames = 1;
        buffer->type = likely_matrix_string;
        result = likely_retain(buffer);
    }

    if (!result)
        result = likely_retain(buffer);

    likely_release(buffer);
    return result;
}

static likely_mat readAsync(const string &fileName, likely_file_type type)
{
    return likely_read(fileName.c_str(), type);
}

static void readRecursive(const string &directory, likely_file_type type, vector<future<likely_mat>> &futures)
{
    DIR *dir = opendir(directory.c_str());
    if (!dir)
        return;

    while (dirent *ent = readdir(dir)) {
        // Skip hidden files and folders
        if (ent->d_name[0] == '.')
            continue;

        stringstream stream;
        stream << directory;
    #ifdef _WIN32
        stream << "\\";
    #else // !_WIN32
        stream << "/";
    #endif // _WIN32
        stream << ent->d_name;
        const string fileName = stream.str();

        if (ent->d_type == DT_REG)
            futures.push_back(async(readAsync, fileName, type));
        else if ((ent->d_type == DT_DIR) || (ent->d_type == DT_LNK))
            readRecursive(fileName, type, futures);
    }

    closedir(dir);
}

likely_mat likely_read(const char *file_name, likely_file_type type)
{
    // Interpret ~ as $HOME
    string fileName = file_name;
    if (fileName[0] == '~')
        fileName = getenv("HOME") + fileName.substr(1);

    // Is it a file?
    if (FILE *fp = fopen(fileName.c_str(), "rb")) {
        fseek(fp, 0, SEEK_END);
        const size_t size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        // Special case, it may already be decoded
        if ((type & likely_file_decoded) && (size >= sizeof(likely_matrix))) {
            likely_matrix header;
            if (fread(&header, 1, sizeof(likely_matrix), fp) == sizeof(likely_matrix)) {
                const size_t bytes = likely_bytes(&header);
                if (sizeof(likely_matrix) + bytes == size) {
                    likely_mat m = likely_new(header.type, header.channels, header.columns, header.rows, header.frames, NULL);
                    if (fread(m->data, 1, bytes, fp) == bytes) return m;
                    else                                       likely_release(m);
                }
            }
        }

        fseek(fp, 0, SEEK_SET);
        likely_mat buffer = likely_new(likely_matrix_u8, 1, size + ((type & likely_file_text) ? 1 : 0), 1, 1, NULL);
        const bool success = (fread(buffer->data, 1, size, fp) == size);
        fclose(fp);
        if (success) {
            return takeAndInterpret(buffer, type);
        } else {
            likely_release(buffer);
            buffer = NULL;
        }
    }

    // Is it a folder?
    vector<future<likely_mat>> futures;
    readRecursive(fileName.c_str(), type, futures);

    // combine
    likely_matrix firstHeader;
    likely_mat result = NULL;
    size_t step = 0;
    bool valid = true;
    for (size_t i=0; i<futures.size(); i++) {
        likely_const_mat image = futures[i].get();
        if ((i == 0) && image) {
            firstHeader = *image;
            step = likely_bytes(&firstHeader);
            result = likely_new(firstHeader.type, firstHeader.channels, firstHeader.columns, firstHeader.rows, firstHeader.frames * futures.size(), NULL);
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
            free(result);
            result = NULL;
        }

        likely_release(image);
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
            likely_release(str);

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
            return likely_retain(mat);
        }
    }

    static likely_const_fun normalize = NULL;
    if (normalize == NULL) {
        likely_const_ast ast = likely_lex_and_parse("(img min range):-> (=> (img min range) (/ (- img min) range).u8 3.channels)", likely_source_lisp);
        likely_env env = likely_new_env_jit();
        normalize = likely_compile(ast->atoms[0], env, likely_matrix_void);
        assert(normalize);
        likely_release_env(env);
        likely_release_ast(ast);
    }

    likely_const_mat min_val = likely_scalar(likely_matrix_f32, min);
    likely_const_mat range_val = likely_scalar(likely_matrix_f32, range);
    likely_mat n = reinterpret_cast<likely_mat (*)(likely_const_mat, likely_const_mat, likely_const_mat)>(normalize->function)(mat, min_val, range_val);
    likely_release(min_val);
    likely_release(range_val);

    if (min_) *min_ = min;
    if (max_) *max_ = max;
    return n;
}

void likely_show(likely_const_mat mat, const char *title)
{
    likely_mat rendered = likely_render(mat, NULL, NULL);
    cv::imshow(title, likelyToOpenCVMat(rendered));
    cv::waitKey();
    likely_release(rendered);
}
