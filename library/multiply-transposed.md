### Multiply transposed
Compare to **[cv::multransposed](http://docs.opencv.org/modules/core/doc/operations_on_arrays.html#multransposed)**.

    multiply-transposed :=
      (src delta) :->
        src.(imitate-size delta.type).(set src).(center delta).(A-transpose-A-generic f64)

| Type  | Single-core | Multi-core |
|-------|-------------|------------|
| u8XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_transposed_f32XY__u8XY_f32X_.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_transposed_f32XY__u8XY_f32X__m.ll)  |
| i16XY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_transposed_f32XY__i16XY_f32X_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_transposed_f32XY__i16XY_f32X__m.ll) |
| f32XY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_transposed_f32XY__f32XY_f32X_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_transposed_f32XY__f32XY_f32X__m.ll) |
| f64XY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_transposed_f64XY__f64XY_f64X_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/multiply_transposed_f64XY__f64XY_f64X__m.ll) |
