### L2 normalization
Compare to **[cv::normalize(NORM_L2)](http://docs.opencv.org/modules/core/doc/operations_on_arrays.html#normalize)**.

    normalize-l2 :=
      src :->
      {
        norm-type := (? (== (& src.type depth) 8) i32 f64)
        norm := 0.norm-type.$
        add-squared-element :=
          (mat t y x c) :->
            norm :<- (+ norm (mat c x y t).f32.sq)

        src:iter-elements add-squared-element
        norm :<- (/ 1 (sqrt norm))

        dst := src.imitate
        (dst src norm) :=>
          dst :<- (* src norm)
      }

#### Generated LLVM IR
| Type    | Single-core | Multi-core |
|---------|-------------|------------|
| u8SCXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/normalize_l2_u8SCXY_u8SCXY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/normalize_l2_u8SCXY_u8SCXY_m.ll)   |
| i16SCXY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/normalize_l2_i16SCXY_i16SCXY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/normalize_l2_i16SCXY_i16SCXY_m.ll) |
| i32CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/normalize_l2_i32CXY_i32CXY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/normalize_l2_i32CXY_i32CXY_m.ll)   |
| f32CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/normalize_l2_f32CXY_f32CXY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/normalize_l2_f32CXY_f32CXY_m.ll)   |
| f64CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/normalize_l2_f64CXY_f64CXY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/normalize_l2_f64CXY_f64CXY_m.ll)   |
