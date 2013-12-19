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

#include "likely_aux.h"
#include "opencv.shim"

using namespace std;

likely_mat likely_read(const char *file_name)
{
    static string previousFileName;
    static likely_mat previousMat = NULL;
    if (previousFileName == file_name)
        return likely_copy(previousMat, 0);

    cv::Mat m = cv::imread(file_name, CV_LOAD_IMAGE_UNCHANGED);
    likely_mat mat = fromCvMat(m, true);

    likely_release(previousMat);
    likely_retain(mat);
    previousMat = mat;
    previousFileName = file_name;
    return mat;
}

void likely_write(likely_const_mat image, const char *file_name)
{
    cv::imwrite(file_name, toCvMat(image));
}

likely_mat likely_decode(likely_const_mat buffer)
{
    return fromCvMat(cv::imdecode(toCvMat(buffer), CV_LOAD_IMAGE_UNCHANGED), true);
}

likely_mat likely_encode(likely_const_mat image, const char *extension)
{
    vector<uchar> buf;
    cv::imencode(string(".") + extension, toCvMat(image), buf);
    return fromCvMat(cv::Mat(buf), true);
}

likely_mat likely_render(likely_const_mat m, double *min_, double *max_)
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

    likely_mat n = likely_new(likely_type_u8, 3, m->columns, m->rows, 1, NULL, 0);
    for (likely_size y=0; y<n->rows; y++) {
        for (likely_size x=0; x<n->columns; x++) {
            for (likely_size c=0; c<3; c++) {
                const double value = likely_element(m, c % m->channels, x, y, 0);
                likely_set_element(n, (value-min)/range, c, x, y, 0);
            }
        }
    }

    if (min_) *min_ = min;
    if (max_) *max_ = max;
    return n;
}

void likely_print(likely_const_mat m)
{
    if (!m) return;
    for (likely_size t=0; t<m->frames; t++) {
        for (likely_size y=0; y<m->rows; y++) {
            cout << (m->rows > 1 ? (y == 0 ? "[" : " ") : "");
            for (likely_size x=0; x<m->columns; x++) {
                for (likely_size c=0; c<m->channels; c++) {
                    cout << likely_element(m, c, x, y, t);
                    if (c != m->channels-1)
                        cout << " ";
                }
                cout << (m->channels > 1 ? ";" : (x < m->columns-1 ? " " : ""));
            }
            cout << ((m->columns > 1) && (y < m->rows-1) ? "\n" : "");
        }
        cout << (m->rows > 1 ? "]\n" : "");
        cout << (t < m->frames-1 ? "\n" : "");
    }
}
