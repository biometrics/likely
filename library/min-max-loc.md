### Minimum & maximum locations
Compare to **[cv::minMaxLoc](http://docs.opencv.org/2.4.8/modules/core/doc/operations_on_arrays.html#minmaxloc)**.

    min-max-loc :=
      src :->
      {
        dst := (new f64XY.(imitate-channels src.type).(imitate-frames src.type) src.channels 3 2 src.frames null)
        width := src.columns
        height := src.rows
        ((dst.channels 1 1 dst.frames) dst src width height) :=>
        {
          current-min-value := src.type.numeric-limit-max.$
          current-min-idx   := 0.$
          current-max-value := src.type.numeric-limit-min.$
          current-max-idx   := 0.$

          check-location :=
            i :->
            {
              current-value := (src c i 0 t)
              (< current-value current-min-value) :?
              {
                current-min-value :<- current-value
                current-min-idx :<- i
              }

              (> current-value current-max-value) :?
              {
                current-max-value :<- current-value
                current-max-idx :<- i
              }
            }

          (iter (-> i (check-location i)) (* width height))

          (dst c 0 0) :<- current-min-value
          (dst c 1 0) :<- (% current-min-idx width)
          (dst c 2 0) :<- (/ current-min-idx width)
          (dst c 0 1) :<- current-max-value
          (dst c 1 1) :<- (% current-max-idx width)
          (dst c 2 1) :<- (/ current-max-idx width)
        }
      }

#### Generated LLVM IR
| Type    | Single-core | Multi-core |
|---------|-------------|------------|
| u8SCXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_u8SCXY.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_u8SCXY_m.ll)  |
| i16SCXY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_i16SCXY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_i16SCXY_m.ll) |
| i32CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_i32CXY.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_i32CXY_m.ll)  |
| f32CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_f32CXY.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_f32CXY_m.ll)  |
| f64CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_f64CXY.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/min_max_loc_f64CXY_f64CXY_m.ll)  |
