The Likely Programming Language
===============================

> Reference manual and standard library.
> -- [@jklontz](https://github.com/jklontz)

Introduction
------------

_Likely_ is a programming language for developing image processing and statistical learning algorithms.
The goal of Likely is concise yet ambitious,

> to revolutionize computer vision algorithm design and deployment.

### Governing Principles
The following sections on just-in-time compilation, portability, live coding, and literate programming introduce the guiding design decisions behind Likely.

#### Just-In-Time Compilation
Likely relies on the _[Low Level Virtual Machine](http://llvm.org/)_ (LLVM) just-in-time (JIT) compiler infrastructure for optimizations traditionally impossible in statically compiled languages.
Algorithms automatically leverage all instruction set extensions and coprocessors available at runtime.
Furthermore, the entire premise of Likely is hinged on the observation that for image recognition applications,

> while algorithms must be generic to handle any matrix data type, at runtime they tend to be repeatedly executed on the same type.

Since images generally pass through the same pipeline at runtime, branching to handle different types is unnecessary and entire code blocks can be eliminated (or never compiled in the first place).
Most importantly however,

> statistical learning moves from an offline model generation step to a compile-time simplification of a function (the learning algorithm) with constant arguments (the training set).

Compile-time knowledge of the statistical model opens the door to [unexplored optimization opportunities](https://github.com/biometrics/likely/issues/20).

### Portability
Likely is written using a portable subset of _ISO C++11_, which means it can [run everywhere LLVM does](http://llvm.org/docs/GettingStarted.html#hardware).
Algorithms execute natively on single CPU threads via the LLVM _Machine Code Just-In-Time_ (MCJIT) compiler, multi-core CPUs via a custom _OpenMP_-like backend, and GPUs and coprocessors via _CUDA_ or _OpenCL_.

| Parallelization | Status |
|-----------------|--------|
| Single-thread   | Done   |
| Multi-thread    | Done   |
| CUDA            | [#24](https://github.com/biometrics/likely/issues/24) |
| OpenCL          | [#25](https://github.com/biometrics/likely/issues/25) |

Likely algorithms can be shipped as minimalist native libraries, written for dynamic just-in-time applications and scripts, integrate into other languages, published as web services, and transcompiled into single _C_ or _JavaScript_ source files.

| Deployment               | Status                                                |
|--------------------------|-------------------------------------------------------|
| Static compiler          | [Done](share/likely/hello_world/hello_world_static.c) |
| JIT compiler             | [Done](share/likely/hello_world/hello_world_jit.c)    |
| REPL shell               | [Done](?show=like)                                    |
| SWIG                     | [#46](https://github.com/biometrics/likely/issues/46) |
| Web services             | [#44](https://github.com/biometrics/likely/issues/44) |
| C transcompiler          | [#51](https://github.com/biometrics/likely/issues/51) |
| JavaScript transcompiler | [#45](https://github.com/biometrics/likely/issues/45) |

### Live Coding
Developing image recognition algorithms is a creative process, and like all creative processes it relies on immediate visceral feedback while interacting with the creative medium.
Image recognition algorithms have parameters, and visualizing their effect is critical for building intuition.
This problem domain and Likely's JIT approach seem well suited for the concept of _live coding_ popularized by [Bret Victor](http://worrydream.com/).
In fact, Likely's accompanying IDE called _Dream_, is [designed from the ground up to support interactive algorithm development](https://www.youtube.com/watch?v=a_hz8wFACVM).

### Literate Programming
Likely is a [literate programming](http://www.literateprogramming.com/) language.
This document, in addition to being the reference manual for the Likely programming language, is also the source code for the Likely standard library.
If you are reading this text on [liblikely.org](http://www.liblikely.org/?show=standard), try clicking the _[View Source](http://raw.github.com/biometrics/likely/gh-pages/library/standard.ll)_ button in the top right corner.

Likely uses [Github Flavored Markdown](https://help.github.com/articles/github-flavored-markdown) (GFM) syntax, and the Likely runtime will automatically extract and execute _code blocks_ found in GFM source files.

Language Reference
------------------

### Prefix Notation
Likely is a Lisp-like language operating on fully-parenthesized Polish prefix notation, or _s-expressions_:

```lisp
(operator operand_1 operand_2 ... operand_N)
```

For example:
```bash
$ likely "(+ 1 2)"
3
```

An important property of s-expressions is that they are equivalent to the compiler's abstract syntax tree (AST).

### AST Manipulation
To improve code readability, Likely has the following special tokens that influence how source code is parsed.

#### __;__ (Comment)
The semicolon and all following characters through the end of the line are excluded from the AST during tokenization.

```lisp
(this is some code) ; This is a comment
```

#### __.__ (Composition)
The period is used for function composition, or more generally [uniform function call syntax](http://www.drdobbs.com/cpp/uniform-function-call-syntax/232700394).

```lisp
x.f       ; Equivalent to (f x)
x.f.g     ; Equivalent to (g (f x))
x.(f y z) ; Equivalent to (f x y z)
7.2       ; Remains 7.2
```

We might call the first two examples _function composition_, and the third example _uniform function call syntax_.
Either way, the left-hand-side (LHS) is inserted as the first operand of the right-hand-side (RHS).
Note, this transformation does not apply to numbers!

### Infix Notation
While everything in Likely can be expressed using prefix notation, as a convenience to developers Likely also offers infix extensions

**TODO: Cleanup and complete the rest of this document.**

| Symbol | Description |

C API
-----
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

Mathematical Constants
----------------------

    e  = (f32 2.71828) ; Euler's number
    pi = (f32 3.14159) ; The ratio of a circle's circumference to its diameter

Unary Functions
---------------

    abs = a -> a < 0 ? (-1 * a) : a
    sq  = a -> a * a

Binary Functions
----------------

    min = (a b) -> a < b ? a : b
    max = (a b) -> a > b ? a : b
