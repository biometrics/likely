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
 * \see \ref likely_guess_file_type \ref likely_read
 */
typedef uint32_t likely_file_type;

/*!
 * \brief \ref likely_file_type bit format.
 */
enum likely_file_type_mask
{
    likely_file_void      = 0x00000000, /*!< \brief It's not a file. */
    likely_file_directory = likely_file_void, /*!< \brief It's a directory, recursively read all the files. */
    likely_file_binary    = 0x00000001, /*!< \brief It's binary data, just read it. */
    likely_file_media     = 0x00000002, /*!< \brief It's an image or video, read and decode it. */
    likely_file_matrix    = 0x00000004, /*!< \brief It's a \ref likely_matrix, memory map it. */
    likely_file_text      = 0x00000008, /*!< \brief It's text, read it and append a \c NULL terminator. */
    likely_file_lisp      = 0x00000010 | likely_file_text, /*!< \brief The text file has plain source code. */
    likely_file_gfm       = 0x00000020 | likely_file_text, /*!< \brief The text file has source code is in [GitHub Flavored Markdown](https://help.github.com/articles/github-flavored-markdown/) code blocks. */
    likely_file_tex       = 0x00000040 | likely_file_text, /*!< \brief The text file has source code is in LaTeX likely environment blocks. */
    likely_file_ir        = 0x00000100 | likely_file_text, /*!< \brief LLVM IR (.ll) */
    likely_file_bitcode   = 0x00000200 | likely_file_binary, /*!< \brief LLVM bitcode (.bc) */
    likely_file_object    = 0x00000400 | likely_file_binary, /*!< \brief Object file (.o) */
    likely_file_assembly  = 0x00000800 | likely_file_binary, /*!< \brief Assembly file (.s) */
    likely_file_guess     = 0xFFFFFFFF /*!< \brief Guess the file type using \ref likely_guess_file_type. */
};

/*!
 * \brief Guess the \ref likely_file_type from a file extension.
 *
 * \par Implementation
 * \snippet src/io.cpp likely_guess_file_type implementation.
 * \param[in] file_name The file name whose extension to interpret.
 * \return Guess of the \ref likely_file_type based on the extension of \p file_name.
 * \see likely_read
 */
LIKELY_EXPORT likely_file_type likely_guess_file_type(const char *file_name);

/*!
 * \brief Convert a string to a \ref likely_file_type.
 *
 * \par Implementation
 * \snippet src/io.cpp likely_file_type_from_string implementation.
 * \param[in] str String to convert to a \ref likely_file_type.
 * \param[out] ok Successful conversion. May be \c NULL.
 * \return A \ref likely_file_type from \p str on success, \ref likely_file_guess otherwise.
 * \remark This function is \ref thread-safe.
 */
LIKELY_EXPORT likely_file_type likely_file_type_from_string(const char *str, bool *ok);

/*!
 * \brief Read a \ref likely_matrix from a file.
 *
 * Supported \ref likely_file_media formats are:
 *
 * | Extension                                                     | Type      |
 * |---------------------------------------------------------------|-----------|
 * | [bmp](http://en.wikipedia.org/wiki/BMP_file_format)           | Image     |
 * | [jpg](http://en.wikipedia.org/wiki/Jpg)                       | Image     |
 * | [png](http://en.wikipedia.org/wiki/Portable_Network_Graphics) | Image     |
 * | [tiff](http://en.wikipedia.org/wiki/Tagged_Image_File_Format) | Image     |
 * |                                                               | Image Set |
 *
 * _Image_ formats expect single-frame matricies.
 * _Video_ formats (not supported yet) expect multi-frame matricies.
 * An _Image Set_ is a folder of images with consistent dimensionality and data type.
 *
 * The presence/absense of \ref likely_multi_channel in \p type dictates whether the returned matrix is a color/grayscale image.
 * \param[in] file_name The name of the file to open and read.
 * \param[in] file_type How to process the file after reading.
 * \param[in] type Expected matrix type.
 * \return Pointer to the new \ref likely_matrix constructed from the file, or \c NULL if the file could not be processed.
 * \remark This function is \ref thread-safe.
 * \see \ref likely_write
 */
LIKELY_EXPORT likely_mat likely_read(const char *file_name, likely_file_type file_type, likely_type type);

/*!
 * \brief Write a \ref likely_matrix to a file.
 *
 * The file extension in \p file_name is used to determine the desired file format.
 * In the case of image data, the input to this function is usually the output from \ref likely_render.
 * \param[in] data The matrix to write.
 * \param[in] file_name The file to write the matrix to.
 * \return \c true if successful, \c false otherwise.
 * \remark This function is \ref thread-safe.
 * \see \ref likely_read
 */
LIKELY_EXPORT bool likely_write(likely_const_mat data, const char *file_name);

/*!
 * \brief Decode a \ref likely_matrix from a buffer.
 *
 * The format of \p buffer is determined automatically.
 *
 * The presence/absense of \ref likely_multi_channel in \p type dictates whether the returned matrix is a color/grayscale image.
 * \param[in] buffer The buffer to decode.
 * \param[in] type Expected matrix type.
 * \return Decoded image if successful, \c NULL otherwise.
 * \remark This function is \ref thread-safe.
 * \see \ref likely_encode
 */
LIKELY_EXPORT likely_mat likely_decode(likely_const_mat buffer, likely_type type);

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
 * \brief Convert to three-channel \ref likely_u8 \ref likely_matrix suitable for saving or displaying.
 *
 * The output from this function is usually the input to \ref likely_write or \ref likely_show.
 * \param[in] mat Matrix to convert.
 * \param[out] min The minimum value in \p mat. May be \c NULL.
 * \param[out] max The maximum value in \p mat. May be \c NULL.
 * \return A three-channel \ref likely_u8 matrix suitable for saving or displaying if successful, \c NULL otherwise.
 * \remark This function is \ref thread-unsafe.
 */
LIKELY_EXPORT likely_mat likely_render(likely_const_mat mat, double *min, double *max);

/*!
 * \brief Display a \ref likely_matrix in a window.
 *
 * Shows \p image in a window, pauses execution until a key is pressed, then hides the window.
 * The input to this function is usually the output from \ref likely_render.
 * \param[in] image Image to display.
 * \param[in] title Window title.
 * \remark This function is \ref thread-safe.
 */
LIKELY_EXPORT void likely_show(likely_const_mat image, const char *title);

/*!
 * \brief Retrieve a matrix from the global table.
 *
 * The value should have been set previously using \ref likely_set_global.
 * \param[in] name Lookup key.
 * \param[in] type Expected value type.
 * \return The matrix for the specified \p name and \p type.
 * \remark This function is \ref thread-safe.
 */
LIKELY_EXPORT likely_mat likely_global(const char *name, likely_type type);

/*!
 * \brief Add a matrix to the global table.
 *
 * This function will assume ownership of \p mat.
 * The value can be later retrieved using \ref likely_global.
 * \param[in] name Lookup key.
 * \param[in] mat Lookup value. May be \c NULL to clear a previous entry.
 * \remark This function is \ref thread-safe.
 */
LIKELY_EXPORT void likely_set_global(const char *name, likely_const_mat mat);

/*!
 * \brief Conditional exit with an error message.
 *
 * Unlike \c assert which calls \c abort, this function calls \c exit.
 * \param[in] condition If \c false, print \a format and abort.
 * \param[in] format <tt>printf</tt>-style error message.
 * \remark This function is \ref thread-safe.
 * \see \ref likely_throw
 */
LIKELY_EXPORT void likely_ensure(bool condition, const char *format, ...);

/*!
 * \brief Ensure that two matricies are approximately equal.
 *
 * Useful for testing against a lossily-compressed baseline image.
 * \param[in] a First matrix.
 * \param[in] b Second matrix.
 * \param[in] error_threshold The allowed cumulative element error relative to the cumulative element sum.
 */
LIKELY_EXPORT void likely_ensure_approximate(likely_const_mat a, likely_const_mat b, float error_threshold);

/** @} */ // end of io

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIKELY_IO_H
