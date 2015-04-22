### Multiply-add
Compare to **[cv::Mat::convertTo](http://docs.opencv.org/2.4.8/modules/core/doc/basic_structures.html#mat-convertto)**.

    multiply-add :=
      (src alpha beta) :->
      {
        dst := src.imitate
        (dst src alpha beta) :=>
        {
          val := (+ (* src alpha) beta)
          dst :<- (? dst.type.is-floating val (round-integer val dst.type))
        }
      }

#### Generated LLVM IR
| Type    | Single-core | Multi-core |
|---------|-------------|------------|
| u8SCXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_add_u8SCXY__u8SCXY_f32_f32_.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_add_u8SCXY__u8SCXY_f32_f32__m.ll)   |
| i16SCXY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_add_i16SCXY__i16SCXY_f32_f32_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_add_i16SCXY__i16SCXY_f32_f32__m.ll) |
| i32CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_add_i32CXY__i32CXY_f32_f32_.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_add_i32CXY__i32CXY_f32_f32__m.ll)   |
| f32CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_add_f32CXY__f32CXY_f32_f32_.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_add_f32CXY__f32CXY_f32_f32__m.ll)   |
| f64CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_add_f64CXY__f64CXY_f64_f64_.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_add_f64CXY__f64CXY_f64_f64__m.ll)   |
