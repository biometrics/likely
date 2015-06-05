### Transpose
Compare to **[cv::transpose](http://docs.opencv.org/modules/core/doc/operations_on_arrays.html#transpose)**.
Where "transpose" is defined in the standard library.

#### Generated LLVM IR
| Type   | Single-core | Multi-core |
|--------|-------------|------------|
| u8SXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_u8SXY_u8SXY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_u8SXY_u8SXY_m.ll)   |
| i16SXY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_i16SXY_i16SXY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_i16SXY_i16SXY_m.ll) |
| i32XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_i32XY_i32XY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_i32XY_i32XY_m.ll)   |
| f32XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_f32XY_f32XY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_f32XY_f32XY_m.ll)   |
| f64XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_f64XY_f64XY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_f64XY_f64XY_m.ll)   |
