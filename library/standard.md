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
If you are reading this text on [liblikely.org](http://www.liblikely.org/?href=standard), try clicking the _[View Source](http://raw.github.com/biometrics/likely/gh-pages/library/standard.md)_ button in the top right corner.

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

### Literals
At the lowest level of interpretation, every atom is either a _number_, _string_, _type_, or _intrinsic operator_.

#### Number
A number is a series of digits with an optional leading negative sign (-) and optional containing decimal point (.).

```likely
42    ; A number
-0.42 ; A number
.42   ; Not a number
0.42- ; Not a number
```

#### String
A string is a series of characters enclosed in quotation marks (").

```likely
"hello world" ; A string
'hello world' ; Not a string
```

#### Type
Types are special keywords indicating how data is represented.
Let's start with a few examples:

```likely
i16 ; 16-bit signed integer scalar
u32 ; 32-bit unsigned integer scalar
f64 ; 64-bit floating-point real scalar
f32C ; 32-bit floating-point real multi-channel matrix
u8XY ; 8-bit unsigned integer multi-column multi-row matrix
```

The general, types are recognized by the following regular expression:

```regex
[uif]\d+A?S?C?X?Y?T?
```

The first character indicates the element type, and is one of:

- **u** - Unsigned integer
- **i** - Signed integer
- **f** - Floating-point real

The next one-or-more decimal characters indicate the element depth, and should generally be a power of two.

The remaining capitalized characters indicate:

- **A** - Type is a pointer (used internally).
- **S** - Perform saturated arithmetic when using this type.
- **C** - Multi-channel matrix.
- **X** - Multi-column matrix.
- **Y** - Multi-row matrix.
- **T** - Multi-frame matrix.

Additionally, the following special cases are recognized as types: **void**, **depth**, **floating**, **array**, **signed**, **saturated**, **element**, **multi-channel**, **multi-column**, **multi-row**, **multi-frame**, **multi-dimension**, **string**, **native**.
These special cases correspond to values of *likely_type_mask* in the C API.

#### Intrinsic Operator
An intrinsic operator is the basic construct for higher order expressions on numbers, strings and types.
The following section details all available intrinsic operators.

### Intrinsic Operators
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

#### Matricies
The fundamental data structure in Likely is a four-dimensional _matrix_.
In decreasing memory spatial locality order, its dimensions are: _channels_, _columns_, _rows_ and _frames_.
These dimensions are often abbreviated _c_, _x_, _y_ and _t_, respectively.
Ownership of matricies is managed automatically using reference counting.

##### (new [_type_ [_channels_ [_columns_ [_rows_ [_frames_ [_data_]]]]]])
Returns a newly allocated matrix with element type _type_, dimensions _channels_, _columns_, _rows_ and _frames_, initialized to _data_.
If _data_ is not specified then the elements will be uninitialized.
If _frames_, _rows_, _columns_ or _channels_ are not specified then they will assume a value of _1_.
If _type_ is not specified then it will assume a value of _f32_ (32-bit floating point).

```likely
uninitialized-color-image := (new u8 3 512 512)
```

##### (channels [_matrix_])
Returns the number of channels in _matrix_ as a native integer.
If _matrix_ is not specified, returns a function which when given a matrix returns the number of channels in the matrix.

```likely
(channels uninitialized-color-image) ; Evaluates to 3
```

##### (columns [_matrix_])
Returns the number of columns in _matrix_ as a native integer.
If _matrix_ is not specified, returns a function which when given a matrix returns the number of columns in the matrix.

```likely
(columns uninitialized-color-image) ; Evaluates to 512
```

##### (rows [_matrix_])
Returns the number of rows in _matrix_ as type a native integer.
If _matrix_ is not specified, returns a function which when given a matrix returns the number of rows in the matrix.

```likely
(rows uninitialized-color-image) ; Evaluates to 512
```

##### (frames [_matrix_])
Returns the number of frames in _matrix_ as type a native integer.
If _matrix_ is not specified, returns a function which when given a matrix returns the number of frames in the matrix.

```likely
(frames uninitialized-color-image) ; Evaluates to 1
```

#### Macros
##### (try _expr_ _fallback_)
Attempts to evaluate _expr_ in the current environment and return the result.
Returns _fallback_ expression in the event of an error.

```likely
(eval (+ 1 1) 3) ; Evaluates to 2
(eval (+ 1) 3)   ; Evaluates to 3
```

#### External Symbols
##### (extern _returnType_ _symbolName_ _parameters_)
References an externally defined symbol.

```likely
((extern i32 "abs" i32) -4) ; Evaluates to 4
```

Standard Library
----------------
### Basic mathematical constants
    null  := 0
    true  := 1
    false := 0
    e  := (f32 2.718281)
    pi := (f32 3.141592)

### Basic unary functions
    not  := (-> a (== a false))
    bool := (-> a (!= a false))
    sq  := (-> a (* a a))
    abs := (-> a (? (< a 0) (* -1 a) a))

### Basic binary functions
    and := (-> (a b) (& a.bool b.bool))
    or  := (-> (a b) (| a.bool b.bool))
    xor := (-> (a b) (^ a.bool b.bool))
    min := (-> (a b) (? (< a b) a b))
    max := (-> (a b) (? (> a b) a b))

### Common types
    string := i8P
    void-pointer := i8P
    file-type := u32

### Matrix information
    elements := (-> mat mat.channels :* mat.columns :* mat.rows :* mat.frames)
    bytes    := (-> mat (/ (+ (* (& mat.type depth) mat.elements) 7) 8))

### Matrix creation
    imitate-size := (-> (mat type) (new type mat.channels mat.columns mat.rows mat.frames))
    imitate := (-> mat (imitate-size mat mat.type))

### Matrix I/O
    read   := (extern u8CXYT "likely_read" (string file-type))
    write  := (extern u8CXYT "likely_write" (u8CXYT string))
    decode := (extern u8CXYT "likely_decode" u8CXYT)
    encode := (extern u8CXYT "likely_encode" (u8CXYT string))
    render := (extern u8CXYT "likely_render" (u8CXYT f64P f64P))
    show   := (extern u8CXYT "likely_show" (u8CXYT string))

### Compiler frontend
    lex := (extern ast "likely_lex" (string file-type))
    parse := (extern ast "likely_parse" ast)
    lex-and-parse := (extern ast "likely_lex_and_parse" (string file-type))

### Compiler backend
    eval := (extern env "likely_eval" (ast env void-pointer void-pointer))

### Type conversion
    cast := (-> (a b) (b.type a)) ; convert a to the type of b

### Thresholding
    threshold-binary          := (-> (input threshold response) (? (> input threshold) response  0))
    threshold-binary-inverse  := (-> (input threshold response) (? (> input threshold) 0         response))
    threshold-truncate        := (-> (input threshold)          (? (> input threshold) threshold input))
    threshold-to-zero         := (-> (input threshold)          (? (> input threshold) input     0))
    threshold-to-zero-inverse := (-> (input threshold)          (? (> input threshold) 0         input))
