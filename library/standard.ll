The Likely Programming Language
===============================
> Language reference and standard library.
> -- [@jklontz](https://github.com/jklontz)

Introduction
------------
_Likely_ is a programming language for developing image processing and statistical learning algorithms.
The goal of Likely is concise yet ambitious, _to revolutionize computer vision algorithm design and deployment_.

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
| REPL shell               | [Done](?href=like)                                    |
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
If you are reading this text on [liblikely.org](http://www.liblikely.org/?href=standard), try clicking the _[View Source](http://raw.github.com/biometrics/likely/gh-pages/library/standard.ll)_ button in the top right corner.

Likely uses [Github Flavored Markdown](https://help.github.com/articles/github-flavored-markdown) (GFM) syntax, and the Likely runtime will automatically extract and execute _code blocks_ found in GFM source files.

Language Reference
------------------
### Prefix Notation
Likely is a Lisp-like language accepting fully-parenthesized Polish prefix notation, or _s-expressions_:

```lisp
(operator operand_1 operand_2 ... operand_N)
```

For example:
```bash
$ likely "(+ 1 2)"
3
```

An important property of s-expressions is that they are equivalent to the compiler's abstract syntax tree.

### Abstract Syntax Tree (AST) Manipulation
Likely recognizes three special tokens that influence how source code is parsed into an AST.
These _optional_ tokens allow a developer to selectively depart from fully-parenthesized Polish prefix notation in order to improve code readability.
These tokens and parenthesis are the _only_ symbols that need not be separated by a space.

| Token | Name    |
|-------|---------|
| ;     | Comment |
| .     | Compose |
| :     | Infix   |

Note that you can print the AST to see how source code is parsed:
```bash
$ likely -ast "1:+ 2"
(+ 1 2)
```

#### ; _Comment_
The semicolon and all following tokens through the end of the line are excluded from the AST.

```lisp
(this is some code) ; This is a comment
(+ 1 2) ; One plus two is three
(sq 3)  ; Three squared is nine
```

#### . _Compose_
The expression to the left-hand-side (LHS) of the period is inserted as the first operand of the expression to the right-hand-side (RHS) of the period.
Compose is _left-associative_.

```lisp
x.f     ; Parsed as (f x)
x.f.g   ; Parsed as (g (f x))
x.(f y) ; Parsed as (f x y)
(f x).g ; Parsed as (g (f x))
(g x.f) ; Parsed as (g (f x))
7.2     ; Parsed as 7.2
3.sq    ; Evaluates to 9
1.(+ 2) ; Evaluates to 3
```

We might call the third example _[uniform function call syntax](http://www.drdobbs.com/cpp/uniform-function-call-syntax/232700394)_.
Note how this transformation does not apply to numbers!

#### : _Infix_
The expression to the RHS of the colon is the operator.
The expression to the LHS of the colon is the first operand.
The second expression to the RHS of the colon is the second operand.
Infix is _right-associative_.

```lisp
x:f y       ; Parsed as (f x y)
z:g x:f y   ; Parsed as (g z (f x y))
x:f (g y)   ; Parsed as (f x (g y))
x.f:h y.g   ; Parsed as (h (f x) (g y))
(g x:f y)   ; Parsed as (g (f x y))
1:+ 2       ; Evaluates to 3
3.sq:+ 4.sq ; Evaluates to 25
```

Note how _infix_ has lower precedence than _compose_.

### Intrinsics
Likely has the following builtin operators.

#### Arithmetic
No surprises here, arithmetic operators have their standard meanings.

##### (+ _lhs_ _rhs_)
The addition of _lhs_ by _rhs_.

```lisp
(+ 2 2)     ; Evaluates to 4
(+ 1.8 2)   ; Evaluates to 3.8
(+ 1.8 2.1) ; Evaluates to 3.9
(+ 1.8 2.2) ; Evaluates to 4
```

##### (- _lhs_ [_rhs_])
The subtraction of _lhs_ by _rhs_.
If _rhs_ is not provided then the negation of _lhs_.

```lisp
(- 3 2)     ; Evaluates to 1
(- 2 3)     ; Evaluates to -1
(- 3.2 2)   ; Evaluates to 1.2
(- 3.2 2.1) ; Evaluates to 1.1
(- 3.2 2.2) ; Evaluates to 1
(- 1)       ; Evaluates to -1
(- -1.1)    ; Evaluates to 1.1
```

##### (* _lhs_ _rhs_)
The multiplication of _lhs_ by _rhs_.

```lisp
(* 1 2)     ; Evaluates to 2
(* 1.3 2)   ; Evaluates to 2.6
(* 1.4 2.1) ; Evaluates to 2.94
(* 1.5 2.0) ; Evaluates to 3
```

##### (/ _lhs_ _rhs_)
The division of _lhs_ by _rhs_.

```lisp
(/ 4 2)     ; Evaluates to 2
(/ 4.5 2)   ; Evaluates to 2.25
(/ 4.5 2.5) ; Evaluates to 1.8
(/ 4.2 2.1) ; Evaluates to 2
```

#### Math
_C_ mathematical functions.

##### (sqrt _x_)
The square root of _x_.

```lisp
(sqrt 2)   ; Evaluates to 1.41421
(sqrt 2.1) ; Evaluates to 1.44914
(sqrt 4)   ; Evaluates to 2
(sqrt 0)   ; Evaluates to 0
```

##### (sin _x_)
The sine of _x_ in radians.

```lisp
(sin 0)         ; Evaluates to 0
(sin 1.570796)  ; Evaluates to 1
(sin -1.570796) ; Evaluates to -1
(sin 0.523599)  ; Evaluates to 0.5
```

##### (cos _x_)
The cosine of _x_ in radians.

```lisp
(cos 0)        ; Evaluates to 1
(cos 3.141593) ; Evaluates to -1
(cos 1.047198) ; Evaluates to 0.5
```

Standard Library
----------------
### Mathematical Constants
    e := (f32 2.71828) ; Euler's number
    pi:= (f32 3.14159) ; The ratio of a circle's circumference to its diameter

### Unary Functions
    abs:= (-> a (? (< a 0) (* -1 a) a))
    sq := (-> a (* a a))

### Binary Functions
    min:= (-> (a b) (? (< a b) a b))
    max:= (-> (a b) (? (> a b) a b))
