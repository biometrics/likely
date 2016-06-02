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

#include <vector>
#include <likely/runtime.h>
#include <opencv2/core.hpp>

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
 * \brief Convert from a \ref likely_type to an OpenCV matrix depth.
 * \param[in] type The \ref likely_matrix::type to convert from.
 * \return The corresponding OpenCV matrix depth, or -1 if \p type is unrepresentable in OpenCV.
 * \remark This function is \ref thread-safe.
 * \see \ref likelyFromOpenCVDepth
 */
inline int likelyToOpenCVDepth(likely_type type)
{
    switch (type & likely_c_type) {
      case likely_u8:  return CV_8U;
      case likely_i8:  return CV_8S;
      case likely_u16: return CV_16U;
      case likely_i16: return CV_16S;
      case likely_i32: return CV_32S;
      case likely_f32: return CV_32F;
      case likely_f64: return CV_64F;
    }
    return -1;
}

/*!
 * \brief Convert from an OpenCV matrix depth to a \ref likely_type.
 * \param[in] depth The \c cv::Mat::depth() to convert from.
 * \return The corresponding \ref likely_type, or \ref likely_void if \p depth is not representable in Likely.
 * \remark This function is \ref thread-safe.
 * \see \ref likelyToOpenCVDepth
 */
inline likely_type likelyFromOpenCVDepth(int depth)
{
    switch (depth) {
      case CV_8U:  return likely_u8;
      case CV_8S:  return likely_i8;
      case CV_16U: return likely_u16;
      case CV_16S: return likely_i16;
      case CV_32S: return likely_i32;
      case CV_32F: return likely_f32;
      case CV_64F: return likely_f64;
    }
    return likely_void;
}

/*!
 * \brief Convert from a \ref likely_matrix to an OpenCV matrix.
 *
 * The inverse of \ref likelyFromOpenCVMat.
 * \param[in] mat The \ref likely_mat to convert from.
 * \param[in] frame The frame in \p mat to convert from.
 * \return An OpenCV matrix referencing \ref likely_matrix::data;
 * \remark This function is \ref thread-safe.
 * \see \ref likelyToOpenCVMats
 */
inline cv::Mat likelyToOpenCVMat(likely_const_mat mat, uint32_t frame = 0)
{
    if (!mat)
        return cv::Mat();
    return cv::Mat((int) mat->rows,
                   (int) mat->columns,
                   CV_MAKETYPE(likelyToOpenCVDepth(mat->type), int(mat->channels)),
                   (void*) (((mat->type & likely_indirect) ? *(char**)mat->data : mat->data) + likely_bytes(mat) * frame / mat->frames)).clone();
}

/*!
 * \brief Convert from an OpenCV matrix to a \ref likely_matrix.
 *
 * The inverse of \ref likelyToOpenCVMat.
 * \param[in] mat The OpenCV matrix to convert from.
 * \return A \ref likely_mat initialized to \c cv::Mat::data.
 * \remark This function is \ref thread-safe.
 * \see \ref likelyFromOpenCVMats
 */
inline likely_mat likelyFromOpenCVMat(const cv::Mat &mat, bool indirect = false)
{
    if (!mat.isContinuous() || !mat.data)
        return NULL;
    likely_type type = likelyFromOpenCVDepth(mat.depth());
    if (indirect) type |= likely_indirect;
    if (mat.channels() > 1) type |= likely_multi_channel;
    if (mat.cols       > 1) type |= likely_multi_column;
    if (mat.rows       > 1) type |= likely_multi_row;
    return likely_new(type, mat.channels(), mat.cols, mat.rows, 1, mat.data);
}

/*!
 * \brief Convert from a \ref likely_matrix to a vector of OpenCV matricies.
 *
 * The inverse of \ref likelyFromOpenCVMats.
 * The output matricies will have size equal to the number of frames in \p mat.
 * \param[in] mat The \ref likely_mat to convert from.
 * \return An OpenCV matrix referencing \ref likely_matrix::data;
 * \remark This function is \ref thread-safe.
 * \see \ref likelyToOpenCVMat
 */
inline std::vector<cv::Mat> likelyToOpenCVMats(likely_const_mat mat)
{
    std::vector<cv::Mat> mats;
    if (!mat)
        return mats;
    for (uint32_t i=0; i<mat->frames; i++)
        mats.push_back(likelyToOpenCVMat(mat, i));
    return mats;
}

/*!
 * \brief Convert from a vector of OpenCV matricies to a \ref likely_matrix.
 *
 * The inverse of \ref likelyToOpenCVMats.
 * The input matricies are expected to be continuous and have the same type and dimensionality.
 * The output matrix will have frames equal to the number of input matricies.
 * \param[in] mats The OpenCV matricies to convert from.
 * \return A \ref likely_mat initialized to the concatenation of all \c cv::Mat::data, or \c NULL if \p mats is empty.
 * \remark This function is \ref thread-safe.
 * \see \ref likelyFromOpenCVMat
 */
inline likely_mat likelyFromOpenCVMats(const std::vector<cv::Mat> &mats)
{
    if (mats.empty())
        return NULL;

    const int depth = mats.front().depth();
    const int channels = mats.front().channels();
    const int columns = mats.front().cols;
    const int rows = mats.front().rows;
    const size_t frames = mats.size();
    likely_type type = likelyFromOpenCVDepth(depth);
    if (channels > 1) type |= likely_multi_channel;
    if (columns  > 1) type |= likely_multi_column;
    if (rows     > 1) type |= likely_multi_row;
    if (frames   > 1) type |= likely_multi_frame;

    const likely_mat m = likely_new(type, channels, columns, rows, uint32_t(frames), NULL);
    const size_t step = (type & likely_depth) * channels * columns * rows / 8;
    for (size_t i=0; i<frames; i++) {
        const cv::Mat &mat = mats[i];
        likely_ensure(mat.isContinuous(), "expected continuous matrix data");
        likely_ensure(mat.depth() == depth, "depth mismatch");
        likely_ensure(mat.channels() == channels, "channel mismatch");
        likely_ensure(mat.cols == columns, "columns mismatch");
        likely_ensure(mat.rows == rows, "rows mismatch");
        memcpy(((m->type & likely_indirect) ? *(char**)m->data : m->data) + i * step, mat.data, step);
    }
    return m;
}

/** @} */ // end of opencv

#endif // LIKELY_OPENCV_HPP
