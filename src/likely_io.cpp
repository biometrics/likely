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
#  define NOMINMAX
#endif

#include <iostream>
#include <string>
#include <opencv2/highgui/highgui.hpp>

#include "likely/likely_backend.h"
#include "likely/likely_frontend.h"
#include "likely/likely_io.h"
#include "opencv.shim"

using namespace std;

likely_mat likely_read(const char *file_name)
{
    static string previousFileName;
    static likely_mat previousMat = NULL;
    if (previousFileName == file_name)
        return likely_copy(previousMat);

    cv::Mat m;
    try {
        m = cv::imread(file_name, CV_LOAD_IMAGE_UNCHANGED);
    } catch (...) {
        return NULL;
    }

    likely_mat mat = fromCvMat(m, true);

    likely_release(previousMat);
    likely_retain(mat);
    previousMat = mat;
    previousFileName = file_name;
    return mat;
}

likely_mat likely_write(likely_const_mat image, const char *file_name)
{
    try {
        cv::imwrite(file_name, toCvMat(image));
    } catch (...) {
        return NULL;
    }
    return (likely_mat) image;
}

likely_mat likely_decode(likely_const_mat buffer)
{
    try {
        return fromCvMat(cv::imdecode(toCvMat(buffer), CV_LOAD_IMAGE_UNCHANGED), true);
    } catch (...) {
        return NULL;
    }
}

likely_mat likely_encode(likely_const_mat image, const char *extension)
{
    vector<uchar> buf;
    try {
        cv::imencode(string(".") + extension, toCvMat(image), buf);
    } catch (...) {
        return NULL;
    }
    return fromCvMat(cv::Mat(buf), true);
}

likely_mat likely_string(const char *string)
{
    return likely_new(likely_type_i8, strlen(string)+1, 1, 1, 1, (likely_buffer) string, true);
}

const char *likely_to_string(likely_const_mat m)
{
    static string result;

    stringstream stream;
    if (m) {
        stream << "{ type=" << likely_type_to_string(m->type);
        if (m->channels > 1) stream << ", channels=" << m->channels;
        if (m->columns  > 1) stream << ", columns="  << m->columns;
        if (m->rows     > 1) stream << ", rows="     << m->rows;
        if (m->frames   > 1) stream << ", frames="   << m->frames;
        stream << ", data=";
        if (likely_elements(m) > 1) stream << "{\n";
        stream << (m->frames > 1 ? "{" : "");
        for (likely_size t=0; t<m->frames; t++) {
            stream << (m->rows > 1 ? "{" : "");
            for (likely_size y=0; y<m->rows; y++) {
                stream << (m->columns > 1 ? "{" : "");
                for (likely_size x=0; x<m->columns; x++) {
                    stream << (m->channels > 1 ? "{" : "");
                    for (likely_size c=0; c<m->channels; c++) {
                        stream << likely_element(m, c, x, y, t);
                        if (c != m->channels-1)
                            stream << ", ";
                    }
                    stream << (m->channels > 1 ? "}" : "");
                    if (x != m->columns-1)
                        stream << ", ";
                }
                stream << (m->columns > 1 ? "}" : "");
                if (y != m->rows-1)
                    stream << ",\n";
            }
            stream << (m->rows > 1 ? "}" : "");
            if (t != m->frames-1)
                stream << ",\n\n";
        }
        stream << (m->frames > 1 ? "}" : "");
        if (likely_elements(m) > 1) stream << "\n}";
        stream << " }";
    }

    result = stream.str();
    return result.c_str();
}

likely_mat likely_print(likely_const_mat m, ...)
{
    va_list ap;
    va_start(ap, m);
    stringstream buffer;
    while (m) {
        buffer << likely_to_string(m);
        m = va_arg(ap, likely_const_mat);
    }
    va_end(ap);
    const string result = buffer.str();
    return likely_new(likely_type_i8, result.length()+1, 1, 1, 1, (likely_buffer)result.c_str(), true);
}

likely_mat likely_render(likely_const_mat m, double *min_, double *max_)
{
    if (!m)
        return NULL;

    double min, max, range;
    if ((m->type & likely_type_mask) != likely_type_u8) {
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
        range = (max - min)/255;
        if ((range >= 0.25) && (range < 1)) {
            max = min + 255;
            range = 1;
        }
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

    static likely_function_n normalize = NULL;
    if (normalize == NULL) {
        likely_const_ast ast = likely_ast_from_string("(kernel (img min range) (cast (/ (- img min) range) u8) (channels 3))");
        likely_env env = likely_new_env();
        normalize = likely_compile_n(ast, env);
        likely_release_env(env);
        likely_release_ast(ast);
    }

    likely_const_mat min_val = likely_scalar(min);
    likely_const_mat range_val = likely_scalar(range);
    likely_const_mat args[] = { m, min_val, range_val };
    likely_mat n = normalize(args);
    likely_release(min_val);
    likely_release(range_val);

    if (min_) *min_ = min;
    if (max_) *max_ = max;
    return n;
}
