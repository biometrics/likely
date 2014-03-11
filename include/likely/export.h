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

#ifndef LIKELY_EXPORT_H
#define LIKELY_EXPORT_H

// Export symbols, don't worry about this
#ifdef _WIN32
#  ifdef LIKELY_LIBRARY
#    define LIKELY_EXPORT __declspec(dllexport)
#  else // !LIKELY_LIBRARY
#    define LIKELY_EXPORT __declspec(dllimport)
#  endif // LIKELY_LIBRARY
#else // !_WIN32
#  ifdef LIKELY_LIBRARY
#    define LIKELY_EXPORT __attribute__((visibility("default")))
#  else // !LIKELY_LIBRARY
#    define LIKELY_EXPORT
#  endif // LIKELY_LIBRARY
#endif // _WIN32

#endif // LIKELY_EXPORT_H
