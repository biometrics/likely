### Matrix multiplication
Compare to **[A*B](http://docs.opencv.org/modules/core/doc/basic_structures.html#matrixexpressions)**.

    matrix-multiplication := mtimes ; Where "mtimes" is defined in the standard library.

#### Generated LLVM IR
| Type   | Single-core | Multi-core |
|--------|-------------|------------|
| f32XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/matrix_multiplication_f32XY__f32XY_f32XY_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/matrix_multiplication_f32XY__f32XY_f32XY__m.ll) |
| f64XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/matrix_multiplication_f64XY__f64XY_f64XY_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/matrix_multiplication_f64XY__f64XY_f64XY__m.ll) |
