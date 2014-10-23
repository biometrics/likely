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

/*!
 * \defgroup opencv OpenCV
 * \brief OpenCV compatibility module (\c likely/opencv.hpp).
 *
 * Unlike the rest of the Likely API, these are \c C++ functions and they depend on [OpenCV](http://opencv.org/).
 * As a result, they follow a \c camelCase naming convention and you must explicitly <tt>\#include \<likely/opencv.hpp\></tt>.
 *
 * All functions are provided inline for easy integration into projects needing to bridge between Likely and OpenCV.
 * @{
 */

/*!
 * \brief Convert from a \ref likely_matrix_type to an OpenCV matrix depth.
 * \param[in] type The \ref likely_matrix::type to convert from.
 * \return The corresponding OpenCV matrix depth, or -1 if \p type is unrepresentable in OpenCV.
 * \remark This function is \ref thread-safe.
 * \see \ref likelyFromOpenCVDepth
 */
inline int likelyToOpenCVDepth(likely_matrix_type type)
{
    switch (type & likely_matrix_c_type) {
      case likely_matrix_u8:  return CV_8U;
      case likely_matrix_i8:  return CV_8S;
      case likely_matrix_u16: return CV_16U;
      case likely_matrix_i16: return CV_16S;
      case likely_matrix_i32: return CV_32S;
      case likely_matrix_f32: return CV_32F;
      case likely_matrix_f64: return CV_64F;
    }
    return -1;
}

/*!
 * \brief Convert from an OpenCV matrix depth to a \ref likely_matrix_type.
 * \param[in] depth The \c cv::Mat::depth() to convert from.
 * \return The corresponding \ref likely_matrix_type, or \ref likely_matrix_void if \p depth is not representable in Likely.
 * \remark This function is \ref thread-safe.
 * \see \ref likelyToOpenCVDepth
 */
inline likely_matrix_type likelyFromOpenCVDepth(int depth)
{
    switch (depth) {
      case CV_8U:  return likely_matrix_u8;
      case CV_8S:  return likely_matrix_i8;
      case CV_16U: return likely_matrix_u16;
      case CV_16S: return likely_matrix_i16;
      case CV_32S: return likely_matrix_i32;
      case CV_32F: return likely_matrix_f32;
      case CV_64F: return likely_matrix_f64;
    }
    return likely_matrix_void;
}

/*!
 * \brief Convert from a \ref likely_matrix to an OpenCV matrix.
 * \note This function performs a \em shallow copy. If you want a deep copy, call \c cv::Mat::clone() afterwards.
 * \param[in] mat The \ref likely_mat to convert from.
 * \return An OpenCV matrix referencing \ref likely_matrix::data;
 * \remark This function is \ref thread-safe.
 * \see \ref likelyFromOpenCVMat
 */
inline cv::Mat likelyToOpenCVMat(likely_const_mat mat)
{
    if (!mat)
        return cv::Mat();
    return cv::Mat((int) mat->rows, (int) mat->columns, CV_MAKETYPE(likelyToOpenCVDepth(mat->type), (int) mat->channels), (void*) mat->data);
}

/*!
 * \brief Convert from an OpenCV matrix to a \ref likely_matrix.
 *
 * This function performs a \em deep copy by necessity.
 * \param[in] mat The OpenCV matrix to convert from.
 * \return A \ref likely_mat initialized to \c cv::Mat::data.
 * \remark This function is \ref thread-safe.
 * \see \ref likelyToOpenCVMat
 */
inline likely_mat likelyFromOpenCVMat(const cv::Mat &mat)
{
    if (!mat.isContinuous() || !mat.data)
        return NULL;
    return likely_new(likelyFromOpenCVDepth(mat.depth()), mat.channels(), mat.cols, mat.rows, 1, mat.data);
}

/** @} */ // end of opencv

#endif // LIKELY_OPENCV_HPP
