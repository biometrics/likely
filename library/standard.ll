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

```likely
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

```likely
(this is some code) ; This is a comment
(+ 1 2) ; One plus two is three
(sq 3)  ; Three squared is nine
```

#### . _Compose_
The expression to the left-hand-side (LHS) of the period is inserted as the first operand of the expression to the right-hand-side (RHS) of the period.
Compose is _left-associative_.

```likely
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

```likely
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
##### (+ _lhs_ _rhs_)
The addition of _lhs_ by _rhs_.

```likely
(+ 2 2)     ; Evaluates to 4
(+ 1.8 2)   ; Evaluates to 3.8
(+ 1.8 2.1) ; Evaluates to 3.9
(+ 1.8 2.2) ; Evaluates to 4
```

##### (- _lhs_ [_rhs_])
The subtraction of _lhs_ by _rhs_.
If _rhs_ is not provided then the negation of _lhs_.

```likely
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

```likely
(* 1 2)     ; Evaluates to 2
(* 1.3 2)   ; Evaluates to 2.6
(* 1.4 2.1) ; Evaluates to 2.94
(* 1.5 2.0) ; Evaluates to 3
```

##### (/ _lhs_ _rhs_)
The division of _lhs_ by _rhs_.

```likely
(/ 4 2)     ; Evaluates to 2
(/ 4.5 2)   ; Evaluates to 2.25
(/ 4.5 2.5) ; Evaluates to 1.8
(/ 4.2 2.1) ; Evaluates to 2
```

##### (% _lhs_ _rhs_)
The remainder after division of _lhs_ by _rhs_.

```likely
(% 7 3)      ; Evaluates to 1
(% 6 3)      ; Evaluates to 0
(% 6.5 3)    ; Evaluates to 0.5
(% -6.5 3)   ; Evaluates to -0.5
(% 6.5 -3)   ; Evaluates to 0.5
(% -6.5 -3)  ; Evaluates to -0.5
(% 6.5 7.5)  ; Evaluates to 6.5
(% 6.5 3.25) ; Evaluates to 0
```

#### Comparison
##### (== _lhs_ _rhs_)
One if _lhs_ equals _rhs_, zero otherwise.

```likely
(== 2 2)   ; Evaluates to 1
(== 2 2.0) ; Evaluates to 1
(== 2 -2)  ; Evaluates to 0
(== 2 2.1) ; Evaluates to 0
```

##### (!= _lhs_ _rhs_)
One if _lhs_ does not equal _rhs_, zero otherwise.

```likely
(!= 3 3)   ; Evaluates to 0
(!= 3 3.0) ; Evaluates to 0
(!= 3 -3)  ; Evaluates to 1
(!= 3 3.1) ; Evaluates to 1
```

##### (< _lhs_ _rhs_)
One if _lhs_ is less than _rhs_, zero otherwise.

```likely
(< 4 5)    ; Evaluates to 1
(< 4 -5.0) ; Evaluates to 0
(< 4 4.0)  ; Evaluates to 0
```

##### (<= _lhs_ _rhs_)
One if _lhs_ is less than or equal to _rhs_, zero otherwise.

```likely
(<= 4 5)    ; Evaluates to 1
(<= 4 -5.0) ; Evaluates to 0
(<= 4 4.0)  ; Evaluates to 1
```

##### (> _lhs_ _rhs_)
One if _lhs_ is greater than _rhs_, zero otherwise.

```likely
(> 6 7)    ; Evaluates to 0
(> 6 -7.0) ; Evaluates to 1
(> 6 6.0)  ; Evaluates to 0
```

##### (>= _lhs_ _rhs_)
One if _lhs_ is greater than or equal to _rhs_, zero otherwise.

```likely
(>= 6 7)    ; Evaluates to 0
(>= 6 -7.0) ; Evaluates to 1
(>= 6 6.0)  ; Evaluates to 1
```

#### Bit Manipulation
##### (& _lhs_ _rhs_)
Bitwise and of _lhs_ and _rhs_.

```likely
(& 1 2) ; Evaluates to 0
(& 1 3) ; Evaluates to 1
```

##### (| _lhs_ _rhs_)
Bitwise or of _lhs_ and _rhs_.

```likely
(| 1 2) ; Evaluates to 3
(| 1 3) ; Evaluates to 3
```

##### (^ _lhs_ _rhs_)
Bitwise exclusive or of _lhs_ and _rhs_.

```likely
(^ 1 2) ; Evaluates to 3
(^ 1 3) ; Evaluates to 2
```

##### (<< _lhs_ _rhs_)
Left shift of _lhs_ by _rhs_ bits.

```likely
(<< 2 0) ; Evaluates to 2
(<< 2 1) ; Evaluates to 4
```

##### (>> _lhs_ _rhs_)
Right shift of _lhs_ by _rhs_ bits.
Arithmetic right shift (sign extension) if _lhs_ is signed, logical right shift (zero extension) otherwise.

```likely
(>> 2 0)  ; Evaluates to 2
(>> 2 1)  ; Evaluates to 1
(>> 2 2)  ; Evaluates to 0
(>> -2 0) ; Evaluates to -2
(>> -2 1) ; Evaluates to -1
```

#### Math
##### (sqrt _x_)
The square root of _x_.

```likely
(sqrt 2)   ; Evaluates to 1.41421
(sqrt 2.1) ; Evaluates to 1.44914
(sqrt 4)   ; Evaluates to 2
(sqrt 0)   ; Evaluates to 0
```

##### (sin _x_)
The sine of an angle of _x_ radians.

```likely
(sin 0)         ; Evaluates to 0
(sin 1.570796)  ; Evaluates to 1
(sin -1.570796) ; Evaluates to -1
(sin 0.523599)  ; Evaluates to 0.5
```

##### (cos _x_)
The cosine of an angle of _x_ radians.

```likely
(cos 0)        ; Evaluates to 1
(cos 3.141593) ; Evaluates to -1
(cos 1.047198) ; Evaluates to 0.5
```

##### (pow _base_ _exponent_)
The _base_ raised to the power _exponent_.

```likely
(pow 2 3)     ; Evaluates to 8
(pow 2 -3)    ; Evaluates to 0.125
(pow -2 3)    ; Evaluates to -8
(pow 1.5 0.5) ; Evaluates to 1.22474
(pow 2 0.5)   ; Evaluates to 1.41421
(pow 4 0.5)   ; Evaluates to 2
(pow 4 0)     ; Evaluates to 1
```

##### (exp _x_)
The base-e exponential function of _x_, which is e raised to the power _x_.

```likely
(exp 0)   ; Evaluates to 1
(exp 1)   ; Evaluates to 2.71828
(exp 1.5) ; Evaluates to 4.48169
```

##### (exp2 _x_)
The base-2 exponential function of _x_, which is 2 raised to the power _x_.

```likely
(exp2 0)   ; Evaluates to 1
(exp2 1)   ; Evaluates to 2
(exp2 0.5) ; Evaluates to 1.41421
(exp2 3)   ; Evaluates to 8
```

##### (log _x_)
The natural logarithm of _x_.

```likely
(log 1)        ; Evaluates to 0
(log 2.718281) ; Evaluates to 1
(log 7.389056) ; Evaluates to 2
(log 0.5)      ; Evaluates to -0.693147
```

##### (log10 _x_)
The common (base-10) logarithm of _x_.

```likely
(log10 1)   ; Evaluates to 0
(log10 10)  ; Evaluates to 1
(log10 100) ; Evaluates to 2
(log10 0.5) ; Evaluates to -0.30103
```

##### (log2 _x_)
The binary (base-2) logarithm of _x_.

```likely
(log2 1)   ; Evaluates to 0
(log2 2)   ; Evaluates to 1
(log2 4)   ; Evaluates to 2
(log2 0.5) ; Evaluates to -1
(log2 10)  ; Evaluates to 3.32193
```

##### (copysign _x_ _y_)
A value with the magnitude of _x_ and the sign of _y_.

```likely
(copysign 3 -1.1) ; Evaluates to -3
(copysign -4.3 2) ; Evaluates to 4.3
```

##### (floor _x_)
The largest integral value that is not greater than _x_.

```likely
(floor 2.3)  ; Evaluates to 2
(floor 3.8)  ; Evaluates to 3
(floor 5.5)  ; Evaluates to 5
(floor -2.3) ; Evaluates to -3
(floor -3.8) ; Evaluates to -4
(floor -5.5) ; Evaluates to -6
```

##### (ceil _x_)
The smallest integral value that is not less than _x_.

```likely
(ceil 2.3)  ; Evaluates to 3
(ceil 3.8)  ; Evaluates to 4
(ceil 5.5)  ; Evaluates to 6
(ceil -2.3) ; Evaluates to -2
(ceil -3.8) ; Evaluates to -3
(ceil -5.5) ; Evaluates to -5
```

##### (trunc _x_)
The nearest integral value that is not larger in magnitude than _x_.

```likely
(trunc 2.3)  ; Evaluates to 2
(trunc 3.8)  ; Evaluates to 3
(trunc 5.5)  ; Evaluates to 5
(trunc -2.3) ; Evaluates to -2
(trunc -3.8) ; Evaluates to -3
(trunc -5.5) ; Evaluates to -5
```

##### (round _x_)
The integral value that is nearest to _x_, with halfway cases rounded away from zero.

```likely
(round 2.3)  ; Evaluates to 2
(round 3.8)  ; Evaluates to 4
(round 5.5)  ; Evaluates to 6
(round -2.3) ; Evaluates to -2
(round -3.8) ; Evaluates to -4
(round -5.5) ; Evaluates to -6
```

#### Matrix I/O
Supported matrix file formats are:

| Extension                                                     | Type      |
|---------------------------------------------------------------|-----------|
| [bmp](http://en.wikipedia.org/wiki/BMP_file_format)           | Image     |
| [jpg](http://en.wikipedia.org/wiki/Jpg)                       | Image     |
| [png](http://en.wikipedia.org/wiki/Portable_Network_Graphics) | Image     |
| [tiff](http://en.wikipedia.org/wiki/Tagged_Image_File_Format) | Image     |
| [zip](http://en.wikipedia.org/wiki/Zip_%28file_format%29)     | Image Set |
| [tar](http://en.wikipedia.org/wiki/Tar_%28computing%29)       | Image Set |
| [gz](http://en.wikipedia.org/wiki/Gzip)                       | Image Set |
| [bz2](http://en.wikipedia.org/wiki/Bzip2)                     | Image Set |

*Image* formats expect single-frame matricies.
*Image Set* formats are a collection of images with consistent dimensionality and data type.
*Video* formats (not supported yet) expect multi-frame matricies.

##### (read _file-name_)
Reads from _file-name_ and returns the decoded matrix.

```likely
lenna:= (read "data/misc/lenna.tiff")
```

##### (write _matrix_ _file-name_)
Encodes _matrix_ based on the extension of _file-name_ and writes the encoded matrix to _file-name_.
Returns the input _matrix_.

```likely
(write lenna "lenna.png")
```

##### (encode _matrix_ _extension_)
Returns the result of encoding _matrix_ using the algorithm specified by _extension_.

```likely
encoded-lenna:= (encode lenna "png")
```

##### (decode _matrix_)
Returns the decoded _matrix_.
Automatically determines the encoding format.

```likely
(decode encoded-lenna)
```

#### Macros
##### (eval _source_)
Returns the result of parsing, compiling, and executing a _source_ code string
in the current environment.

```likely
(eval "(+ 1 1)") ; Evaluates to 2
(eval "(+ 1)")   ; Evaluation error
```

Standard Library
----------------
### Mathematical Constants
    e := (f32 2.718281) ; Euler's number
    pi:= (f32 3.141592) ; The ratio of a circle's circumference to its diameter

### Unary Functions
    abs:= (-> a (? (< a 0) (* -1 a) a))
    sq := (-> a (* a a))

### Binary Functions
    and:= (-> (a b) (& (!= a 0) (!= b 0)))
    or := (-> (a b) (| (!= a 0) (!= b 0)))
    xor:= (-> (a b) (^ (!= a 0) (!= b 0)))
    min:= (-> (a b) (? (< a b) a b))
    max:= (-> (a b) (? (> a b) a b))
    cast:= (-> (a b) (b.type a)) ; cast a to the type of b

### Thresholding
    threshold-binary         := (-> (input threshold response) (? (> input threshold) response  0))
    threshold-binary-inverse := (-> (input threshold response) (? (> input threshold) 0         response))
    threshold-truncate       := (-> (input threshold)          (? (> input threshold) threshold input))
    threshold-to-zero        := (-> (input threshold)          (? (> input threshold) input     0))
    threshold-to-zero-inverse:= (-> (input threshold)          (? (> input threshold) 0         input))
