### Fused multiply-add
Compare to **[cv::Mat::convertTo](http://docs.opencv.org/2.4.8/modules/core/doc/basic_structures.html#mat-convertto)**.

    fused-multiply-add :=
      (src alpha beta) :->
      {
        dst := src.imitate
        (dst src alpha beta) :=>
          dst :<- (+ (* src alpha) beta)
      }

#### Generated LLVM IR
| Type    | Single-core | Multi-core |
|---------|-------------|------------|
| u8SCXY  | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_u8SCXY__u8SCXY_f32_f32_.ll)   | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_u8SCXY__u8SCXY_f32_f32__m.ll)   |
| i16SCXY | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_i16SCXY__i16SCXY_f32_f32_.ll) | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_i16SCXY__i16SCXY_f32_f32__m.ll) |
| i32CXY  | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_i32CXY__i32CXY_f32_f32_.ll)   | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_i32CXY__i32CXY_f32_f32__m.ll)   |
| f32CXY  | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_f32CXY__f32CXY_f32_f32_.ll)   | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_f32CXY__f32CXY_f32_f32__m.ll)   |
| f64CXY  | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_f64CXY__f64CXY_f64_f64_.ll)   | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_f64CXY__f64CXY_f64_f64__m.ll)   |
