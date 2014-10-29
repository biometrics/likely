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

#ifndef LIKELY_IO_H
#define LIKELY_IO_H

#include <likely/runtime.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*!
 * \defgroup io I/O
 * \brief Read, write and render matricies (\c likely/io.h).
 * @{
 */

/*!
 * \brief How to read a file from disk.
 *
 * Available options are listed in \ref likely_file_type_mask.
 * \see \ref likely_read
 */
typedef uint32_t likely_file_type;

/*!
 * \brief \ref likely_file_type bit format.
 */
enum likely_file_type_mask
{
    likely_file_decoded = 0x00000001, /*!< \brief The file is a \ref likely_matrix, do not decode it. */
    likely_file_encoded = 0x00000002, /*!< \brief The file is an image, image set or video, decode it. */
    likely_file_binary  = likely_file_decoded | likely_file_encoded, /*!< \brief The file is either \ref likely_file_decoded or \ref likely_file_encoded. */
    likely_file_text    = 0x00000004, /*!< \brief The file is text, do not decode it. */
};

/*!
 * \brief Read a \ref likely_matrix from a file.
 * \param[in] file_name The name of the file to open and read.
 * \param[in] type How to process the file after reading.
 * \return Pointer to the new \ref likely_matrix constructed from the file, or \c NULL if the file could not be processed.
 * \remark This function is \ref thread-safe.
 * \see \ref likely_write
 */
LIKELY_EXPORT likely_mat likely_read(const char *file_name, likely_file_type type);

/*!
 * \brief Write a \ref likely_matrix to a file.
 *
 * The file extension in \p file_name is used to determine the desired file format.
 * \param[in] image The matrix to write.
 * \param[in] file_name The file to write the matrix to.
 * \return \p image if successful, \c NULL otherwise.
 * \remark This function is \ref thread-safe.
 * \see \ref likely_read
 */
LIKELY_EXPORT likely_mat likely_write(likely_const_mat image, const char *file_name);

/*!
 * \brief Decode a \ref likely_matrix from a buffer.
 *
 * The format of \p buffer is determined automatically.
 * \param[in] buffer The buffer to decode.
 * \return Decoded image if successful, \c NULL otherwise.
 * \remark This function is \ref thread-safe.
 * \see \ref likely_encode
 */
LIKELY_EXPORT likely_mat likely_decode(likely_const_mat buffer);

/*!
 * \brief Encode a \ref likely_matrix to a buffer.
 *
 * \param[in] image The image to encode.
 * \param[in] extension The desired encoding format.
 * \return Encoded buffer if successful, \c NULL otherwise.
 * \remark This function is \ref thread-safe.
 * \see \ref likely_decode
 */
LIKELY_EXPORT likely_mat likely_encode(likely_const_mat image, const char *extension);

/*!
 * \brief Convert a \ref likely_matrix to a \ref likely_string suitable for printing.
 * \param[in] mat Matrix to stringify.
 * \return Stringified matrix.
 * \remark This function is \ref thread-safe.
 * \see \ref likely_to_string_n
 */
LIKELY_EXPORT likely_mat likely_to_string(likely_const_mat mat);

/*!
 * \brief Convert an array of \ref likely_matrix to a \ref likely_string suitable for printing.
 * \param[in] mats Vector of matricies to stringify.
 * \param[in] n Length of \p mats.
 * \return Stringified matrix.
 * \remark This function is \ref thread-safe.
 * \see \ref likely_to_string \ref likely_to_string_va
 */
LIKELY_EXPORT likely_mat likely_to_string_n(likely_const_mat *mats, size_t n);

/*!
 * \brief Convert a list of \ref likely_matrix to a \ref likely_string suitable for printing.
 * \param[in] mat <tt>NULL</tt>-terminated list of matricies to stringify.
 * \return Stringified matrix.
 * \remark This function is \ref thread-safe
 * \see \ref likely_to_string_n
 */
LIKELY_EXPORT likely_mat likely_to_string_va(likely_const_mat mat, ...);

/*!
 * \brief Convert to three-channel \ref likely_matrix_u8 \ref likely_matrix suitable for displaying.
 * \param[in] mat Matrix to convert.
 * \param[out] min The minimum value in \p mat. May be \c NULL.
 * \param[out] max The maximum value in \p mat. May be \c NULL.
 * \return A three-channel \ref likely_matrix_u8 matrix suitable for displaying.
 * \remark This function is \ref thread-unsafe.
 * \see \ref likely_show
 */
LIKELY_EXPORT likely_mat likely_render(likely_const_mat mat, double *min, double *max);

/*!
 * \brief Displays a \ref likely_matrix in a window.
 *
 * Calls \ref likely_render on \p mat automatically.
 * Pauses execution until a key is pressed, then hides the window.
 * \param[in] mat Matrix to display.
 * \param[in] title Window title.
 * \remark This function is \ref thread-safe.
 */
LIKELY_EXPORT void likely_show(likely_const_mat mat, const char *title);

/*!
 * \brief Compute the MD5 hash of \ref likely_matrix::data.
 * \param[in] mat \ref likely_matrix::data to compute the MD5 hash of.
 * \return A new matrix where containing the 16-byte MD5 hash.
 * \remark This function is \ref thread-safe.
 */
LIKELY_EXPORT likely_mat likely_md5(likely_const_mat mat);

/** @} */ // end of io

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIKELY_IO_H
