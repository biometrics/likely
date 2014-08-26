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

#### ; Comment
The semicolon and all following tokens through the end of the line are excluded from the AST.

```lisp
(this is some code) ; This is a comment
```

#### . Unary Composition
The token to the left-hand-side (LHS) of the period is inserted as the first operand of the token to the right-hand-side (RHS) of the period.

```lisp
x.f       ; Equivalent to (f x)
x.f.g     ; Equivalent to (g (f x))
x.(f y z) ; Equivalent to (f x y z)
7.2       ; Remains 7.2
```

We might call the first two examples _function composition_, and the third example _[uniform function call syntax](http://www.drdobbs.com/cpp/uniform-function-call-syntax/232700394)_.
Note this transformation does not apply to numbers!

#### : Binary Composition
The token on the LHS of the colon is inserted as the first operand of the second token to the RHS of the colon.
The token on the RHS of the colon is inserted as the operator of the second token to the RHS of the colon.

```lisp
x:f y     ; Equivalent to (f x y)
x:f y:g z ; Equivalent to (g (f x y) z)
x:f (g y) ; Equivalent to (f x (g y))
```

We might call these examples _infix notation_

Mathematical Constants
----------------------
    (= e  (f32 2.71828)) ; Euler's number
    (= pi (f32 3.14159)) ; The ratio of a circle's circumference to its diameter

Unary Functions
---------------
    abs:= (-> a (? (< a 0) (* -1 a) a))
    sq := (-> a (* a a))

Binary Functions
----------------
    min:= (-> (a b) (? (< a b) a b))
    max:= (-> (a b) (? (> a b) a b))
