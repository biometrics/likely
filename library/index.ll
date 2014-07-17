Likely
======

 > A just-in-time Lisp for image recognition and heterogenous architectures.
 > -- [@jklontz](https://github.com/jklontz)

## Introduction
Skip to the good stuff: **[installation](?show=README.md)**, **[hello world](?show=share/likely/hello_world/README.ll)**, **[tutorial](?show=tutorial)**, **[demos](?show=demos)**.

### Principles
 - Heterogeneous hardware architecture support
 - Efficient syntax for feature representation and statistical learning
 - Immediate visual feedback during algorithm development
 - Embeddable into other projects and languages
 - Free open source software

### Help
We try to keep this document complete and correct. However, should you run into trouble, please reach out on our [mailing list](https://groups.google.com/forum/#!forum/likely-dev) and report bugs on our [issue tracker](https://github.com/biometrics/likely/issues).

### About
Likely is designed for **[literate programming](http://en.wikipedia.org/wiki/Literate_programming)**, including the [standard library](?show=standard). Likely uses [Github Flavored Markdown](https://help.github.com/articles/github-flavored-markdown) (GFM) syntax, and the Likely runtime will automatically extract and execute _code blocks_ found in GFM source files.

Likely is **licensed under [Apache 2.0](LICENSE.txt)**, meaning it's *free for academic and commercial use*.

### Background
The software engineering of a complex system is often facilitated by a _Domain Specific Language_ (DSL) whose syntax is designed to efficiently solve problems encountered in the domain. While many technical communities rely on DSLs, there currently exists no DSL designed for image recognition. As a consequence, most libraries and applications are written in either [_C++_ or _MATLAB_](?show=motivation), neither of which respects both the engineer's time and the hardware's compute capability.

> A just-in-time compiled DSL for image recognition is essential to express algorithms that are currently too complex to implement in native languages and too inefficient to execute in scripting languages.

### Portability
Likely is built on top of the _Low Level Virtual Machine_ (LLVM) compiler infrastructure using a portable subset of _ISO C++11_, which means it can **[run everywhere](http://llvm.org/docs/GettingStarted.html#hardware)** LLVM does. Algorithms execute natively on single CPU threads via the LLVM _Machine Code Just-In-Time_ (MCJIT) compiler, multi-core CPUs via a custom _OpenMP_-like backend, and GPUs and coprocessors via _CUDA_ or _OpenCL_.

| Backend     | Status |
|-------------|--------|
| Single-core | Done   |
| Multi-core  | Done   |
| CUDA        | [#24](https://github.com/biometrics/likely/issues/24) |
| OpenCL      | [#25](https://github.com/biometrics/likely/issues/25) |

Algorithms expressed in Likely **deploy everywhere**, including minimalist native libraries, dynamic just-in-time applications, light-weight scripts, high-level language integration, web services, and pure _C_ and _JavaScript_.

| Frontend        | Status                                                |
|-----------------|-------------------------------------------------------|
| Static compiler | [Done](share/likely/hello_world/hello_world_static.c) |
| JIT compiler    | [Done](share/likely/hello_world/hello_world_jit.c)    |
| REPL shell      | [Done](?show=like)                                    |
| SWIG            | [#46](https://github.com/biometrics/likely/issues/46) |
| Web services    | [#44](https://github.com/biometrics/likely/issues/44) |
| C               | [#51](https://github.com/biometrics/likely/issues/51) |
| JavaScript      | [#45](https://github.com/biometrics/likely/issues/45) |

### Speed
Likely relies on the LLVM compiler infrastructure to optimize function execution. Algorithms automatically leverage all instruction set extensions and co-processors available at run time. Just-in-time (JIT) compilation also enables optimizations traditionally impossible in statically compiled languages. In fact, the entire premise of Likely is hinged on the observation that for image recognition applications

> while algorithms must be written generically to handle any matrix type, at runtime they tend to be executed repeatedly on the same type.

The repeated execution of an algorithm with the same matrix type means that branching to handle different types is unnecessary, entire loops and code blocks can be eliminated, and many values that would be runtime parameters instead become compile-time constants. Most importantly,

> statistical learning changes from an offline process to a compile-time simplification.

## C API
C/C++ projects should **#include <likely.h>** and link against the **likely** library. **LikelyConfig.cmake** is provided in **share/likely/** as a convenience to CMake developers. Likely definitions are prefixed with **likely_** and use a **lowercase_underscore** naming convention. Consider taking a moment to skim the header files: [likely_runtime.h](include/likely/likely_runtime.h), [likely_compiler.h](include/likely/likely_compiler.h), [likely_script.h](include/likely/likely_script.h), [likely_io.h](include/likely/likely_io.h).

### The Matrix Struct
The **likely_matrix**, or _matrix_, is the principal data type for input and output to all function calls:

| Field    | Type           | Description                       |
|----------|----------------|-----------------------------------|
| d_ptr    | likely_private | Used for internal bookkeeping     |
| data     | likely_data    | Pointer to the buffer of elements |
| channels | likely_size    | Matrix dimension                  |
| columns  | likely_size    | Matrix dimension                  |
| rows     | likely_size    | Matrix dimension                  |
| frames   | likely_size    | Matrix dimension                  |
| type     | likely_type    | Matrix type                       |

The last five fields (_channels_, _columns_, _rows_, _frames_, and _type_) are collectively referred to as the matrix _header_. In contrast to most image processing libraries which tend to feature 3-dimensional matrices (channels, columns, rows), Likely includes a fourth dimension, frames, in order to facilitate processing videos or collections of images.

#### Element Access
By convention, element layout in the data buffer with resepect to decreasing spatial locality is _channel_, _column_, _row_, _frame_. Thus an element at channel _c_, column _x_, row _y_, and frame _t_, can be retrieved like:
```cpp
float likely_get_element(likely_matrix m, likely_size c, likely_size x, likely_size y, likely_size t)
{
    likely_size columnStep = m->channels;
    likely_size rowStep = m->channels * columnStep;
    likely_size frameStep = m->rows * rowStep;
    likely_size index = t*frameStep + y*rowStep + x*columnStep + c;
    assert(likely_type(m) == likely_type_f32);
    return reinterpret_cast<float*>(m->data)[index];
}
```

Convenience functions **likely_element** and **likely_set_element** are provided for individual element access. These functions are inefficient for iterating over a large numbers of elements due to the repeated index calculations, and their use is suggested only for debugging purposes or when the matrix is known to be small.

#### Matrix Type
As suggested earlier, **likely_type** plays a critical role in determining how to process matricies. The type is a 4-byte _bit field_ that encodes what data type the matrix is and how/where it should be processed. It also specifies knowledge used to optimized code generation and by removing unneccesary indexing, looping, and branching.

Here is the layout of the bits in **likey_type**:

| Field         | Bits | Mask       |
|---------------|------|------------|
| depth         | 8    | 0x000000FF |
| signed        | 1    | 0x00000100 |
| floating      | 1    | 0x00000200 |
| parallel      | 1    | 0x00000400 |
| heterogeneous | 1    | 0x00000800 |
| multi_channel | 1    | 0x00001000 |
| multi_column  | 1    | 0x00002000 |
| multi_row     | 1    | 0x00004000 |
| multi_frame   | 1    | 0x00008000 |
| saturation    | 1    | 0x00010000 |
| reserved      | 15   | 0xFFFE0000 |

Convenience functions **likely__field_** and **likely\_set__field_** are provided for querying and editing the type.
