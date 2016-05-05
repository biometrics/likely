### Minimum & maximum locations
Compare to **[cv::minMaxLoc](http://docs.opencv.org/2.4.8/modules/core/doc/operations_on_arrays.html#minmaxloc)**.

    min-max-loc :=
      src :->
      {
        current-min-value := src.element-type.numeric-limit-max.$
        current-min-idx   := 0.$
        current-max-value := src.element-type.numeric-limit-min.$
        current-max-idx   := 0.$

        check-location :=
          i :->
          {
            current-value := (src i)
            (< current-value (current-min-value)) :?
            {
              current-min-value :<- current-value
              current-min-idx :<- i
            }

            (> current-value (current-max-value)) :?
            {
              current-max-value :<- current-value
              current-max-idx :<- i
            }
          }

        (iter check-location src.elements)

        dst := (f64XY 1 3 2)
        (dst 0 0 0) :<- (current-min-value)
        (dst 0 1 0) :<- (% (current-min-idx) src.columns)
        (dst 0 2 0) :<- (/ (current-min-idx) src.columns)
        (dst 0 0 1) :<- (current-max-value)
        (dst 0 1 1) :<- (% (current-max-idx) src.columns)
        (dst 0 2 1) :<- (/ (current-max-idx) src.columns)
        dst
      }

#### Generated LLVM IR
| Type    | Single-core | Multi-core |
|---------|-------------|------------|
| u8SCXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_u8SCXY.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_u8SCXY_m.ll)  |
| i16SCXY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_i16SCXY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_i16SCXY_m.ll) |
| i32CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_i32CXY.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_i32CXY_m.ll)  |
| f32CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_f32CXY.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_f32CXY_m.ll)  |
| f64CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_f64CXY.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_f64CXY_m.ll)  |
