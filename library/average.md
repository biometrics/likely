### Average
Compare to **[cv::reduce(CV_REDUCE_AVG)](http://docs.opencv.org/modules/core/doc/operations_on_arrays.html#reduce)**.

    average :=
      src :->
      {
        avg := (new src.type.depth-atleast-32.floating.not-multi-row src.channels src.columns 1 src.frames null)
        avg :=>
          avg :<- 0
        (avg src) :+>
          avg :<- (+ avg src)
        norm := (/ 1 src.rows.floating)
        (avg norm) :=>
          avg :<- (* avg norm)
      }

#### Generated LLVM IR
| Type  | Single-core | Multi-core |
|-------|-------------|------------|
| u8XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_u8XY.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_u8XY_m.ll)  |
| i16XY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_i16XY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_i16XY_m.ll) |
| f32XY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_f32XY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f32X_f32XY_m.ll) |
| f64XY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f64X_f64XY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/average_f64X_f64XY_m.ll) |
