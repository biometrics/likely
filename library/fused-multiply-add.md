### Fused multiply-add
Compare to **[cv::Mat::convertTo](http://docs.opencv.org/2.4.8/modules/core/doc/basic_structures.html#mat-convertto)**.

    fused-multiply-add :=
      (src alpha beta) :->
      {
        dst-type := src.type.floating
        dst := src.(imitate-size dst-type)
        (dst src alpha beta) :=>
          dst :<- (+ (* src.dst-type alpha.dst-type) beta.dst-type)
      }

#### Generated LLVM IR
| Type    | Single-core | Multi-core |
|---------|-------------|------------|
| u8SCXY  | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_f32CXY__u8SCXY_double_double_.ll)  | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_f32CXY__u8SCXY_double_double__m.ll)  |
| u16SCXY | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_f32CXY__u16SCXY_double_double_.ll) | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_f32CXY__u16SCXY_double_double__m.ll) |
| i32CXY  | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_f32CXY__i32CXY_double_double_.ll)  | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_f32CXY__i32CXY_double_double__m.ll)  |
| f32CXY  | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_f32CXY__f32CXY_double_double_.ll)  | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_f32CXY__f32CXY_double_double__m.ll)  |
| f64CXY  | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_f64CXY__f64CXY_double_double_.ll)  | [View](https://s3.amazonaws.com/liblikely/benchmarks/fused_multiply_add_f64CXY__f64CXY_double_double__m.ll)  |
