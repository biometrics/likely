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

#ifndef LIKELY_AUX_H
#define LIKELY_AUX_H

#include <likely/likely_core.h>

#ifdef __cplusplus
extern "C" {
#endif

// Matrix I/O
LIKELY_EXPORT likely_mat likely_read(const char *file_name);
LIKELY_EXPORT void likely_write(likely_const_mat image, const char *file_name);
LIKELY_EXPORT likely_mat likely_decode(likely_const_mat buffer);
LIKELY_EXPORT likely_mat likely_encode(likely_const_mat image, const char *extension);
LIKELY_EXPORT likely_mat likely_render(likely_const_mat m, double *min, double *max); // Return a 888 matrix for visualization

// Debug
LIKELY_EXPORT void likely_print(likely_const_mat m);

#ifdef __cplusplus
}
#endif

#endif // LIKELY_AUX_H
