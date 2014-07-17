Motivation
----------
This section considers some of the drawbacks in using the two incumbent languages in the field, **C++** and **MATLAB**, to help motivate why an image recognition DSL is important. This document is a work in progress, so please [let us know](https://groups.google.com/forum/#!forum/likely-dev) if you disagree with it!

#### Why not C++?
As image recognition engineers, we desire a curated set of image processing and statistical learning functions from which to compose higher-order algorithms. Of fundamental concern is the construction of leak-free abstractions that enable complex reasoning. To do so

> functions must be expressed generically, and only once, to a compiler capable of automatically determining _how_ and _where_ given source code that says _what_.

Despite the multi-paradigm virtues of C++, it is extraordinarily difficult to provide library routines that are both truly generic and efficient. To illustrate this point, consider the following library function for adding two vectors:

```cpp
void add(const float *a, const float *b, float *c, unsigned int n) {
    for (unsigned int i = 0; i < n; i++)
        c[i] = a[i] + b[i];
}
```

##### Aliasing
While this function is simple, it is neither particularly fast nor generic. A popular assumption made in numerical libraries is that pointers never _alias_. In other words, variables _a_, _b_ and _c_ point to distinct non-overlapping regions in memory. The C++ language specification states that pointers of the same type may alias, so the compiler must insert a runtime check when vectorizing loops. While C has since introduced the _restrict_ keyword to specify pointers that do not alias, this keyword is not supported in the C++ standard and is implemented instead through compiler-specific annotations:

```cpp
void add(const float * __restrict a, const float * __restrict b, float * __restrict c, unsigned int n) {
    for (unsigned int i = 0; i < n; i++)
        c[i] = a[i] + b[i];
}
```

Similar to [Fortran and pure functional langauges](http://en.wikipedia.org/wiki/Pointer_aliasing#Aliasing_and_re-ordering), when writing image recognition algorithms we're interested in a compiler that assumes pointers don't alias, instead of relying on an often-forgotten language extension to explicitly say so.

##### Generics
Most image processing operations make sense to express for a variety of data types, so we might try to generalize our function using templates:

```cpp
template <typename T>
void add(const T *a, const T *b, T *c, unsigned int n) {
    for (unsigned int i = 0; i < n; i++)
        c[i] = a[i] + b[i];
}
```

However, what if we would like _a_ or _b_ to be a numerical constant? Or _c_ to be a different type? This author knows of know way to do this without resorting to partial template specialization, which runs counter to our principle of only implementing a function only once.

##### Saturation
Arithmetic overflow is a common problem encoutered in image processing. So common in fact that OpenCV [always](http://docs.opencv.org/modules/core/doc/intro.html#saturation-arithmetics) uses it for 8- and 16-bit integer arithmetic.

```cpp
template<> inline uchar saturate_cast<uchar>(int v)
{ return (uchar)((unsigned)v <= UCHAR_MAX ? v : v > 0 ? UCHAR_MAX : 0); }
```

But what if your algorithm is designed to avoid overflows and this _select_ instruction is unnecessary?

##### Parallelization
What if we'd like our function to execute in parallel on multiple CPU cores? Or on a GPU if it is available? OpenMP and OpenCL provide cross-platform solutions here, but not without having to mark-up or maintain multiple copies of our function. Image recognition is often embarasingly parallel, and we'd like the compiler parallelize our algorithms automatically.

##### Conclusion
The critical reader will no doubt observe that C++ offers reasonable solutions to each of the individual considerations mentioned above. However, in reality an algorithm designer must wrestle with all of these considerations simultaneously, and it takes a herculean effort to do so. Consider the software engineering involved in implementing and maintaining OpenCV's optimized generic matrix addition and subtraction [arithm_op](https://raw.github.com/Itseez/opencv/master/modules/core/src/arithm.cpp) function. Yet it's still a leaky abstraction because it prevents arguably the single most important optimization that can be made when writing image processing algorithms: avoiding memory I/O when chaining together multiple arithmetic operations. Writing generic code while juggling all of these optimization considerations requires considerable cognitive load and developement time that would be better spent designing algorithms with a compiler that automatically handled them.

### Why not MATLAB?
**To Be Continued**

### Why not _X_?
How well does _X_ support the following features?
 - Alias analysis
 - Generic types
 - Saturation arithmetic
 - Vectorization
 - Multi-core processors
 - Co-processors
 - Simple pixel access
 - Dense, cropped, and sparse matricies
 - Integral images
 - Model training
 - Algorithm visualization and interaction
