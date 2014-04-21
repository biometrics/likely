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
#include <string>
#include <opencv2/highgui/highgui.hpp>
#include <archive.h>
#include <archive_entry.h>

#include "likely/backend.h"
#include "likely/frontend.h"
#include "likely/io.h"
#include "likely/opencv.hpp"

using namespace std;

likely_mat likely_read(const char *file_name)
{
    // Interpret ~ as $HOME
    string fileName = file_name;
    if (fileName[0] == '~')
        fileName = getenv("HOME") + fileName.substr(1);

    likely_mat m = NULL;

    // Is it a matrix?
    if (fileName.substr(fileName.size()-3) == ".lm") {
        if (FILE *fp = fopen(fileName.c_str(), "rb")) {
            likely_size bytes;
            if (fread(&bytes, sizeof(bytes), 1, fp) == 1) {
                fseek(fp, 0, SEEK_SET);
                m = (likely_mat) malloc(bytes);
                if (m && (fread(m, 1, bytes, fp) != bytes)) {
                    free(m);
                    m = NULL;
                }
            }
            fclose(fp);
        }
    }

    // Is it an image?
    try {
        m = likely::fromCvMat(cv::imread(fileName.c_str(), CV_LOAD_IMAGE_UNCHANGED));
    } catch (...) { }

    // Is it an archive?
    if (m == NULL) {
        vector<likely_const_mat> images;
        { // unarchive and decode
            archive *a = archive_read_new();
            archive_read_support_format_all(a);
            archive_read_support_filter_all(a);
            int r = archive_read_open_filename(a, fileName.c_str(), 10240);
            while (r == ARCHIVE_OK) {
                struct archive_entry *entry;
                r = archive_read_next_header(a, &entry);
                if (r == ARCHIVE_OK) {
                    likely_mat encodedImage = likely_new(likely_type_u8, 1, archive_entry_size(entry), 1, 1, NULL);
                    archive_read_data(a, encodedImage->data, encodedImage->columns);
                    images.push_back(likely_decode(encodedImage));
                    likely_release(encodedImage);
                }
            }
            archive_read_close(a);
            archive_read_free(a);
        }

        // combine
        likely_const_mat first = images.empty() ? NULL : images.front();
        bool valid = !!first;
        for (size_t i=0; i<images.size(); i++)
            valid = valid && images[i]
                    && (images[i]->type     == first->type)
                    && (images[i]->channels == first->channels)
                    && (images[i]->columns  == first->columns)
                    && (images[i]->rows     == first->rows)
                    && (images[i]->frames   == first->frames);

        if (valid) {
            m = likely_new(first->type, first->channels, first->columns, first->rows, first->frames * images.size(), NULL);
            const size_t step = likely_bytes(first);
            for (size_t i=0; i<images.size(); i++)
                memcpy(m->data + i*step, images[i]->data, step);
        }

        for (likely_const_mat n : images)
            likely_release(n);
    }
    return m;
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
    return (likely_mat) image;
}

likely_mat likely_decode(likely_const_mat buffer)
{
    try {
        return likely::fromCvMat(cv::imdecode(likely::toCvMat(buffer), CV_LOAD_IMAGE_UNCHANGED));
    } catch (...) {
        return NULL;
    }
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

likely_mat likely_to_string(likely_const_mat m, int header)
{
    if (!m) return NULL;
    if ((likely_data(m->type) == likely_type_i8) && !m->data[likely_elements(m)-1])
        return likely_retain(m); // Special case where matrix encodes a string

    stringstream stream;
    if (header) {
        stream << "(";
        likely_mat str = likely_type_to_string(m->type);
        stream << str->data;
        likely_release(str);
    }
    if (header >= 0) {
        stream << " ";
        if (likely_elements(m) > 1)
            stream << "(\n";
        stream << (m->frames > 1 ? "(" : "");
        for (likely_size t=0; t<m->frames; t++) {
            stream << (m->rows > 1 ? "(" : "");
            for (likely_size y=0; y<m->rows; y++) {
                stream << (m->columns > 1 ? "(" : "");
                for (likely_size x=0; x<m->columns; x++) {
                    stream << (m->channels > 1 ? "(" : "");
                    for (likely_size c=0; c<m->channels; c++) {
                        stream << likely_element(m, c, x, y, t);
                        if (c != m->channels-1)
                            stream << " ";
                    }
                    stream << (m->channels > 1 ? ")" : "");
                    if (x != m->columns-1)
                        stream << " ";
                }
                stream << (m->columns > 1 ? ")" : "");
                if (y != m->rows-1)
                    stream << "\n";
            }
            stream << (m->rows > 1 ? ")" : "");
            if (t != m->frames-1)
                stream << "\n\n";
        }
        stream << (m->frames > 1 ? ")" : "");
        if (likely_elements(m) > 1)
            stream << "\n)";
    }
    if (header) {
        const bool frames   =            (m->frames   > 1);
        const bool rows     = frames  || (m->rows     > 1);
        const bool columns  = rows    || (m->columns  > 1);
        const bool channels = columns || (m->channels > 1);
        if (channels) stream << " (" << m->channels;
        if (columns ) stream << " "  << m->columns;
        if (rows    ) stream << " "  << m->rows;
        if (frames  ) stream << " "  << m->frames;
        if (channels) stream << ")";
        stream << ")";
    }

    return likely_string(stream.str().c_str());
}

likely_mat likely_print(likely_const_mat m, ...)
{
    va_list ap;
    va_start(ap, m);
    stringstream buffer;
    while (m) {
        likely_mat str = likely_to_string(m, false);
        buffer << str->data;
        likely_release(str);
        m = va_arg(ap, likely_const_mat);
    }
    va_end(ap);
    return likely_string(buffer.str().c_str());
}

likely_mat likely_render(likely_const_mat m, double *min_, double *max_)
{
    if (!m)
        return NULL;

    double min, max, range;
    if (likely_data(m->type) != likely_type_u8) {
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

    static likely_function_3 normalize = NULL;
    if (normalize == NULL) {
        likely_const_ast ast = likely_ast_from_string("(=> (img min range) (u8 (img - min) / range) (channels 3))", false);
        likely_env env = likely_new_env();
        normalize = reinterpret_cast<likely_function_3>(likely_compile(ast, env, likely_type_void));
        assert(normalize);
        likely_release_env(env);
        likely_release_ast(ast);
    }

    likely_const_mat min_val = likely_scalar(min, likely_type_f32);
    likely_const_mat range_val = likely_scalar(range, likely_type_f32);
    likely_mat n = normalize(m, min_val, range_val);
    likely_release(min_val);
    likely_release(range_val);

    if (min_) *min_ = min;
    if (max_) *max_ = max;
    return n;
}

static void default_show_callback(likely_const_mat m, const char *name, void *)
{
    if (!m) return;
    likely_mat n = likely_render(m, NULL, NULL);
    cv::imshow(strlen(name) > 0 ? name : "Likely", likely::toCvMat(n));
    cv::waitKey();
    likely_release(n);
}

static likely_show_callback ShowCallback = default_show_callback;
static void *ShowContext = NULL;

void likely_set_show_callback(likely_show_callback callback, void *context)
{
    ShowCallback = callback;
    ShowContext = context;
}

void likely_show(likely_const_mat m, const char *name)
{
    if (ShowCallback) {
        ShowCallback(m, name, ShowContext);
    } else {
        likely_mat str = likely_to_string(m, true);
        printf("%s\n", str->data);
        likely_release(str);
    }
}
