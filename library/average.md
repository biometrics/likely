### Average
Compare to **[cv::reduce(CV_REDUCE_AVG)](http://docs.opencv.org/modules/core/doc/operations_on_arrays.html#reduce)**.

    average := average-row ; Where "average-row" is defined in the standard library.

#### Generated LLVM IR
| Type   | Single-core | Multi-core |
|--------|-------------|------------|
| u8SXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_u8SXY.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_u8SXY_m.ll)  |
| i16SXY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_i16SXY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_i16SXY_m.ll) |
| f32XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_f32XY.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_f32XY_m.ll)  |
| f64XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f64X_f64XY.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f64X_f64XY_m.ll)  |
