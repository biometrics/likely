### GEMM
Compare to **[cv::gemm](http://docs.opencv.org/modules/core/doc/operations_on_arrays.html#gemm)**.

    gemm :=
      (src1 src2 alpha src3 beta) :->
      {
        (assume (== src2.rows src1.columns))
        (assume (== src1.rows src3.rows))
        (assume (== src2.columns src3.columns))
        dst := (src3.type 1 src3.columns src3.rows)
        len := src1.columns
        (dst src1 src2 alpha src3 beta len) :=>
        {
          native-type := dst.element-type
          dot-product := (dot (-> i (src1 0 i y))
                              (-> i (src2 0 x i))
                              len
                              native-type)
          dst :<- (+ (* alpha dot-product).native-type (* beta src3).native-type)
        }
      }

#### Generated LLVM IR
| Type   | Single-core | Multi-core |
|--------|-------------|------------|
| f32XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/gemm_f32XY__f32XY_f32XY_f64_f32XY_f64_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/gemm_f32XY__f32XY_f32XY_f64_f32XY_f64__m.ll) |
| f64XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/gemm_f64XY__f64XY_f64XY_f64_f64XY_f64_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/gemm_f64XY__f64XY_f64XY_f64_f64XY_f64__m.ll) |
