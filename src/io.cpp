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
#include <curl/curl.h> // include before <archive.h> to avoid issues on Windows
#include <archive.h>
#include <archive_entry.h>

#include "likely/backend.h"
#include "likely/io.h"
#include "likely/opencv.hpp"

using namespace std;

bool likely_decoded(likely_file_type type) { return likely_get_bool(type, likely_file_decoded); }
void likely_set_decoded(likely_file_type *type, bool decoded) { likely_set_bool(type, decoded, likely_file_decoded); }
bool likely_encoded(likely_file_type type) { return likely_get_bool(type, likely_file_encoded); }
void likely_set_encoded(likely_file_type *type, bool encoded) { likely_set_bool(type, encoded, likely_file_encoded); }
bool likely_text(likely_file_type type) { return likely_get_bool(type, likely_file_text); }
void likely_set_text(likely_file_type *type, bool text) { likely_set_bool(type, text, likely_file_text); }
bool likely_url(likely_file_type type) { return likely_get_bool(type, likely_file_url); }
void likely_set_url(likely_file_type *type, bool url) { likely_set_bool(type, url, likely_file_url); }

static likely_mat takeAndInterpret(likely_mat buffer, likely_type type)
{
    likely_mat result = NULL;
    if (!result && likely_decoded(type)) {
        if (likely_bytes(buffer) >= sizeof(likely_matrix)) {
            likely_mat header = (likely_mat) buffer->data;
            if (sizeof(likely_matrix) + likely_bytes(header) == likely_bytes(buffer))
                result = likely_copy(header);
        }
    }

    if (!result && likely_encoded(type))
        result = likely_decode(buffer);

    if (!result && likely_text(type)) {
        buffer->data[likely_bytes(buffer)-1] = 0;
        likely_set_data(&buffer->type, likely_matrix_i8);
        result = likely_retain(buffer);
    }

    if (!result)
        result = likely_retain(buffer);

    likely_release(buffer);
    return result;
}

static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    vector<char> *userpc = static_cast<vector<char>*>(userp);
    char *bufferc = static_cast<char*>(buffer);
    userpc->insert(userpc->end(), bufferc, bufferc+(size*nmemb));
    return size*nmemb;
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
        if (likely_decoded(type) && (size >= sizeof(likely_matrix))) {
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
        likely_mat buffer = likely_new(likely_matrix_u8, 1, size + (likely_text(type) ? 1 : 0), 1, 1, NULL);
        if (fread(buffer->data, 1, size, fp) == size) {
            return takeAndInterpret(buffer, type);
        } else {
            likely_release(buffer);
            buffer = NULL;
        }
    }

    // Is it a URL?
    if (likely_url(type)) {
        static bool init = false;
        if (!init) {
            curl_global_init(CURL_GLOBAL_ALL);
            init = true;
        }

        vector<char> buffer;
        if (CURL *curl = curl_easy_init()) {
            curl_easy_setopt(curl, CURLOPT_URL, file_name);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
            CURLcode result = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (result != CURLE_OK)
                buffer.clear();
        }

        if (!buffer.empty())
            return takeAndInterpret(likely_new(likely_matrix_u8, 1, buffer.size(), 1, 1, buffer.data()), type);
    }

    return NULL;
}

likely_mat likely_write(likely_const_mat image, const char *file_name)
{
    const size_t len = strlen(file_name);
    if ((len < 3) || strcmp(&file_name[len-3], ".lm")) {
        try {
            cv::imwrite(file_name, likely::toCvMat(image));
        } catch (...) {
            return NULL;
        }
    } else {
        if (FILE *fp = fopen(file_name, "wb")) {
            likely_size bytes = sizeof(likely_matrix) + likely_bytes(image);
            likely_size ref_count = 1;
            fwrite(&bytes, sizeof(likely_size), 1, fp);
            fwrite(&ref_count, sizeof(likely_size), 1, fp);
            fwrite(reinterpret_cast<likely_size const*>(image)+2, bytes-2*sizeof(likely_size), 1, fp);
            fclose(fp);
        } else {
            return NULL;
        }
    }
    return likely_retain(image);
}

static likely_mat decodeAndRelease(likely_const_mat buffer)
{
    likely_mat result = likely_decode(buffer);
    likely_release(buffer);
    return result;
}

likely_mat likely_decode(likely_const_mat buffer)
{
    likely_mat m = NULL;

    try {
        m = likely::fromCvMat(cv::imdecode(likely::toCvMat(buffer), CV_LOAD_IMAGE_UNCHANGED));
    } catch (...) {}

    // Is it an archive?
    if (m == NULL) {
        vector<future<likely_mat>> futures;
        { // unarchive and decode
            archive *a = archive_read_new();
            archive_read_support_format_all(a);
            archive_read_support_filter_all(a);
            int r = archive_read_open_memory(a, (void*) buffer->data, likely_bytes(buffer));
            while (r == ARCHIVE_OK) {
                struct archive_entry *entry;
                r = archive_read_next_header(a, &entry);
                if (r == ARCHIVE_OK) {
                    likely_mat encodedImage = likely_new(likely_matrix_u8, 1, archive_entry_size(entry), 1, 1, NULL);
                    archive_read_data(a, encodedImage->data, encodedImage->columns);
                    futures.push_back(async(decodeAndRelease, encodedImage));
                }
            }
            archive_read_close(a);
            archive_read_free(a);
        }

        // combine
        likely_matrix first;
        size_t step = 0;
        bool valid = true;
        for (size_t i=0; i<futures.size(); i++) {
            likely_const_mat image = futures[i].get();
            if ((i == 0) && image) {
                first = *image;
                step = likely_bytes(&first);
                m = likely_new(first.type, first.channels, first.columns, first.rows, first.frames * futures.size(), NULL);
            }

            valid = valid
                    && image
                    && (image->type     == first.type)
                    && (image->channels == first.channels)
                    && (image->columns  == first.columns)
                    && (image->rows     == first.rows)
                    && (image->frames   == first.frames);

            if (valid) {
                memcpy(m->data + i*step, image->data, step);
            } else if (m) {
                free(m);
                m = NULL;
            }

            likely_release(image);
        }
    }

    return m;
}

likely_mat likely_encode(likely_const_mat image, const char *extension)
{
    vector<uchar> buf;
    try {
        cv::imencode(string(".") + extension, likely::toCvMat(image), buf);
    } catch (...) {
        return NULL;
    }
    return likely::fromCvMat(cv::Mat(buf));
}

bool likely_is_string(likely_const_mat m)
{
    return m && (likely_data(m->type) == likely_matrix_i8) && !m->data[likely_elements(m)-1];
}

likely_mat likely_to_hex(likely_const_mat m)
{
    char hex_str[] = "0123456789abcdef";
    const likely_size bytes = likely_bytes(m);
    likely_mat n = likely_new(likely_matrix_i8, 2*likely_bytes(m)+1, 1, 1, 1, NULL);
    for (likely_size i=0; i<bytes; i++) {
        n->data[2*i+0] = hex_str[(m->data[i] >> 4) & 0x0F];
        n->data[2*i+1] = hex_str[(m->data[i] >> 0) & 0x0F];
    }
    n->data[2*bytes] = 0;
    return n;
}

likely_mat likely_print(likely_const_mat m)
{
    return likely_print_n(&m, 1);
}

likely_mat likely_print_n(likely_const_mat *mv, size_t n)
{
    stringstream buffer;
    for (size_t i=0; i<n; i++) {
        likely_const_mat m = mv[i];
        if (!m) {
            // skip it
        } else if (likely_is_string(m)) {
            buffer << m->data;
        } else if (likely_elements(m) == 1) {
            buffer << likely_element(m, 0, 0, 0, 0);
        } else {
            buffer << "(";
            likely_mat str = likely_type_to_string(m->type);
            buffer << str->data << " ";
            likely_release(str);

            buffer << (m->frames > 1 ? "(" : "");
            for (likely_size t=0; t<m->frames; t++) {
                buffer << (m->rows > 1 ? "(" : "");
                for (likely_size y=0; y<m->rows; y++) {
                    buffer << (m->columns > 1 ? "(" : "");
                    for (likely_size x=0; x<m->columns; x++) {
                        buffer << (m->channels > 1 ? "(" : "");
                        for (likely_size c=0; c<m->channels; c++) {
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

likely_mat likely_print_va(likely_const_mat m, ...)
{
    va_list ap;
    va_start(ap, m);
    vector<likely_const_mat> mv;
    while (m) {
        mv.push_back(m);
        m = va_arg(ap, likely_const_mat);
    }
    return likely_print_n(mv.data(), mv.size());
}

likely_mat likely_render(likely_const_mat m, double *min_, double *max_)
{
    if (!m)
        return NULL;

    double min, max, range;
    if (likely_data(m->type) != likely_matrix_u8) {
        min = numeric_limits<double>::max();
        max = -numeric_limits<double>::max();
        for (likely_size t=0; t<m->frames; t++) {
            for (likely_size y=0; y<m->rows; y++) {
                for (likely_size x=0; x<m->columns; x++) {
                    for (likely_size c=0; c<m->channels; c++) {
                        const double value = likely_element(m, c, x, y, t);
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
        if (m->channels == 3) {
            if (min_) *min_ = min;
            if (max_) *max_ = max;
            return likely_retain(m);
        }
    }

    static likely_const_fun normalize = NULL;
    if (normalize == NULL) {
        likely_const_ast ast = likely_ast_from_string("(=> (img min range) (/ (- img min) range).u8 3.channels)", false);
        likely_env env = likely_new_env_jit();
        normalize = likely_compile(ast->atoms[0], env, likely_matrix_void);
        assert(normalize);
        likely_release_env(env);
        likely_release_ast(ast);
    }

    likely_const_mat min_val = likely_scalar(likely_matrix_f32, min);
    likely_const_mat range_val = likely_scalar(likely_matrix_f32, range);
    likely_mat n = reinterpret_cast<likely_function_3>(normalize->function)(m, min_val, range_val);
    likely_release(min_val);
    likely_release(range_val);

    if (min_) *min_ = min;
    if (max_) *max_ = max;
    return n;
}

void likely_show(likely_const_mat m, const char *title)
{
    likely_mat rendered = likely_render(m, NULL, NULL);
    cv::imshow(title, likely::toCvMat(rendered));
    cv::waitKey();
    likely_release(rendered);
}

void likely_show_callback(likely_const_env env, void *context)
{
    if (!env || !context) return;
    likely_show(env->result, likely_get_symbol_name(env->ast));
}
