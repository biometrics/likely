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

likely_matrix likely_read(const char *file_name)
{
    static string previousFileName;
    static likely_matrix previousMat = NULL;
    if (previousFileName == file_name)
        return likely_copy(previousMat, 1);

    cv::Mat m;
    try {
        m = cv::imread(file_name, CV_LOAD_IMAGE_UNCHANGED);
    } catch (...) {
        return NULL;
    }

    likely_matrix mat = fromCvMat(m, true);

    likely_release(previousMat);
    likely_retain(mat);
    previousMat = mat;
    previousFileName = file_name;
    return mat;
}

void likely_write(const likely_matrix image, const char *file_name)
{
    try {
        cv::imwrite(file_name, toCvMat(image));
    } catch (...) {
        return;
    }
}

likely_matrix likely_decode(const likely_matrix buffer)
{
    try {
        return fromCvMat(cv::imdecode(toCvMat(buffer), CV_LOAD_IMAGE_UNCHANGED), true);
    } catch (...) {
        return NULL;
    }
}

likely_matrix likely_encode(const likely_matrix image, const char *extension)
{
    vector<uchar> buf;
    try {
        cv::imencode(string(".") + extension, toCvMat(image), buf);
    } catch (...) {
        return NULL;
    }
    return fromCvMat(cv::Mat(buf), true);
}

likely_matrix likely_render(const likely_matrix m, double *min_, double *max_)
{
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
            return likely_copy(m, 0);
        }
    }

    static likely_function_n normalize = likely_compile_n(likely_ast_from_string("(lambda (img min range) (cast (/ (- img min) range) u8) (channels 3))"));
    likely_matrix min_val = likely_scalar(min);
    likely_matrix range_val = likely_scalar(range);
    const likely_matrix args[] = { m, min_val, range_val };
    likely_matrix n = normalize(args);
    likely_release(min_val);
    likely_release(range_val);

    if (min_) *min_ = min;
    if (max_) *max_ = max;
    return n;
}

const char *likely_print(const likely_matrix m)
{
    static string result;

    stringstream stream;
    if (m) {
        stream << "{ type=" << likely_type_to_string(m->type)
               << ", channels=" << m->channels
               << ", columns=" << m->columns
               << ", rows=" << m->rows
               << ", frames=" << m->frames
               << ", data={\n";
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
        stream << "\n} }";
    }

    result = stream.str();
    return result.c_str();
}
