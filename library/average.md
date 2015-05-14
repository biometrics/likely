### Average
Compare to **[cv::reduce(CV_REDUCE_AVG)](http://docs.opencv.org/modules/core/doc/operations_on_arrays.html#reduce)**.

    average :=
      src :->
      {
        sum := (new src.type.depth-atleast-32.floating.not-multi-row src.channels src.columns 1 src.frames null)
        sum :=>
          sum :<- 0
        (sum src) :+>
          sum :<- (+ sum src)
        norm := (/ 1 src.rows.floating)
        (sum norm) :=>
          sum :<- (* sum norm)
      }

#### Generated LLVM IR
| Type | Single-core | Multi-core |
|------|-------------|------------|
| f32X | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_u8XY.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_u8XY_m.ll)  |
| f32X | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_i16XY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_i16XY_m.ll) |
| f32X | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_f32XY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_f32XY_m.ll) |
| f64X | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f64X_f64XY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f64X_f64XY_m.ll) |
