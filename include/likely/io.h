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
 * \see likely_file_type_mask
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
    likely_file_url     = 0x00000008 /*!< \brief The file is not on the local file system, download it instead. */
};

LIKELY_EXPORT likely_mat likely_read(const char *file_name, likely_size type);
LIKELY_EXPORT likely_mat likely_write(likely_const_mat image, const char *file_name);
LIKELY_EXPORT likely_mat likely_decode(likely_const_mat buffer);
LIKELY_EXPORT likely_mat likely_encode(likely_const_mat image, const char *extension);

// Matrix Visualization
LIKELY_EXPORT likely_mat likely_to_hex(likely_const_mat m);
LIKELY_EXPORT likely_mat likely_print(likely_const_mat m);
LIKELY_EXPORT likely_mat likely_print_n(likely_const_mat *mv, size_t n);
LIKELY_EXPORT likely_mat likely_print_va(likely_const_mat m, ...);
LIKELY_EXPORT likely_mat likely_render(likely_const_mat m, double *min, double *max); // Return an 888 matrix for visualization
LIKELY_EXPORT void likely_show(likely_const_mat m, const char *title);
LIKELY_EXPORT void likely_show_callback(likely_const_env env, void *); // Useable as a likely_repl_callback

/** @} */ // end of io

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIKELY_IO_H
