#ifndef LIKELY_H
#define LIKELY_H

#include <likely/backend.h>
#include <likely/export.h>
#include <likely/frontend.h>
#include <likely/io.h>
#include <likely/runtime.h>

/*!
 * \mainpage Likely API
 *
 * Welcome to the Likely API documentation!
 * Both the \c likely and \c dream exectuables are written on top of this interface, so anything they can do you can do too!
 *
 * \section getting_started Getting Started
 * Likely has a \c C API.
 * Your project should <b><tt>\#include \<likely.h\></tt></b> and link against <b><tt>likely</tt></b>.
 * Note that all Likely symbols are prefixed with \c likely_ and follow a \c lowercase_underscore naming convention.
 * <i>[CMake](http://www.cmake.org/)</i> users may appreciate \c LikelyConfig.cmake provided in \c share/likely/.
 *
 * Start reading \ref hello_world.
 */

/*!
 * \page hello_world Hello World
 * \brief Source code for the \c hello_world_jit application.
 *
 * A good entry point for learning about the Likely API.
 * Source code in <tt>share/likely/hello_world/hello_world_jit.c</tt> reproduced below.
 * \snippet share/likely/hello_world/hello_world_jit.c hello_world_jit implementation.
 */

#endif // LIKELY_H
