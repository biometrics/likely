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

#include <likely/likely_runtime.h>

#ifdef __cplusplus
extern "C" {
#endif

// Matrix I/O
LIKELY_EXPORT likely_matrix likely_read(const char *file_name);
LIKELY_EXPORT likely_matrix likely_write(const likely_matrix image, const char *file_name);
LIKELY_EXPORT likely_matrix likely_decode(const likely_matrix buffer);
LIKELY_EXPORT likely_matrix likely_encode(const likely_matrix image, const char *extension);

// Matrix Visualization
LIKELY_EXPORT likely_matrix likely_render(const likely_matrix m, double *min, double *max); // Return a 888 matrix for visualization
LIKELY_EXPORT const char *likely_print(const likely_matrix m); // Return value managed internally and guaranteed until the next call to this function

#ifdef __cplusplus
}
#endif

#endif // LIKELY_IO_H
