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

#include "likely/opencv.hpp"

namespace likely
{

int typeToDepth(likely_type type)
{
    switch (likely_data(type)) {
      case likely_type_u8:  return CV_8U;
      case likely_type_i8:  return CV_8S;
      case likely_type_u16: return CV_16U;
      case likely_type_i16: return CV_16S;
      case likely_type_i32: return CV_32S;
      case likely_type_f32: return CV_32F;
      case likely_type_f64: return CV_64F;
    }
    assert(!"Unsupported matrix depth.");
    return 0;
}

likely_type depthToType(int depth)
{
    switch (depth) {
      case CV_8U:  return likely_type_u8;
      case CV_8S:  return likely_type_i8;
      case CV_16U: return likely_type_u16;
      case CV_16S: return likely_type_i16;
      case CV_32S: return likely_type_i32;
      case CV_32F: return likely_type_f32;
      case CV_64F: return likely_type_f64;
    }
    assert(!"Unsupported matrix depth.");
    return likely_type_void;
}

cv::Mat toCvMat(likely_const_mat m)
{
    return cv::Mat((int)m->rows, (int)m->columns, CV_MAKETYPE(typeToDepth(m->type), (int)m->channels), (void*)m->data);
}

likely_mat fromCvMat(const cv::Mat &src)
{
    if (!src.isContinuous() || !src.data)
        return likely_new(likely_type_void, 0, 0, 0, 0, NULL);
    return likely_new(depthToType(src.depth()), src.channels(), src.cols, src.rows, 1, src.data);
}

} // namespace likely
