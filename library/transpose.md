### Transpose
Compare to **[cv::transpose](http://docs.opencv.org/modules/core/doc/operations_on_arrays.html#transpose)**.

    transpose :=
      src :->
      {
        dst := src.imitate
        (dst src) :=>
          (dst c x y) :<- (src c y x)
      }

#### Generated LLVM IR
| Type  | Single-core | Multi-core |
|-------|-------------|------------|
| u8XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_u8XY_u8XY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_u8XY_u8XY_m.ll)   |
| i16XY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_i16XY_i16XY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_i16XY_i16XY_m.ll) |
| i32XY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_i32XY_i32XY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_i32XY_i32XY_m.ll) |
| f32XY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_f32XY_f32XY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_f32XY_f32XY_m.ll) |
| f64XY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_f64XY_f64XY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/transpose_f64XY_f64XY_m.ll) |
