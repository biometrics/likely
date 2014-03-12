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

#ifndef LIKELY_OPENCV_HPP
#define LIKELY_OPENCV_HPP

#include <likely/runtime.h>
#include <opencv2/core/core.hpp>

namespace likely
{
    LIKELY_EXPORT int typeToDepth(likely_type type);
    LIKELY_EXPORT likely_type depthToType(int depth);
    LIKELY_EXPORT cv::Mat toCvMat(likely_const_mat m);
    LIKELY_EXPORT likely_mat fromCvMat(const cv::Mat &src);
} // namespace likely

#endif // LIKELY_OPENCV_HPP
