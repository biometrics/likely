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

typedef void (*likely_show_callback)(likely_const_mat m, likely_const_ast, void *context);

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Matrix I/O
LIKELY_EXPORT likely_mat likely_read(const char *file_name, bool decode);
LIKELY_EXPORT likely_mat likely_write(likely_const_mat image, const char *file_name);
LIKELY_EXPORT likely_mat likely_decode(likely_const_mat buffer);
LIKELY_EXPORT likely_mat likely_encode(likely_const_mat image, const char *extension);

// Matrix Visualization
LIKELY_EXPORT likely_mat likely_to_string(likely_const_mat m, int header);
LIKELY_EXPORT likely_mat likely_print(likely_const_mat m, ...);
LIKELY_EXPORT likely_mat likely_render(likely_const_mat m, double *min, double *max); // Return an 888 matrix for visualization
LIKELY_EXPORT void likely_set_show_callback(likely_show_callback callback, void *context);
LIKELY_EXPORT void likely_show(likely_const_mat m, likely_const_ast ast);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIKELY_IO_H
