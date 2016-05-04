### L2 normalization
Compare to **[cv::normalize(NORM_L2)](http://docs.opencv.org/modules/core/doc/operations_on_arrays.html#normalize)**.

    normalize-l2 :=
      src :->
      {
        norm := 0.f64.$
        add-squared-element :=
          e :->
            norm :<- (+ (norm) e.f64.sq)
        src:iter-elements add-squared-element
        norm :<- (/ 1 (sqrt (norm)))
        norm := (src.element-type (norm))

        dst := src.imitate
        (dst src norm) :=>
          dst :<- (* src norm)
      }

#### Generated LLVM IR
| Type    | Single-core | Multi-core |
|---------|-------------|------------|
| f32CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/normalize_l2_f32CXY_f32CXY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/normalize_l2_f32CXY_f32CXY_m.ll) |
| f64CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/normalize_l2_f64CXY_f64CXY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/normalize_l2_f64CXY_f64CXY_m.ll) |
