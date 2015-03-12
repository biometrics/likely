### Binary threshold
Compare to **[cv::threshold(THRESHOLD_BINARY)](http://docs.opencv.org/2.4.8/modules/imgproc/doc/miscellaneous_transformations.html#threshold)**.

    binary-threshold :=
      (src thresh maxval) :->
      {
        dst := src.imitate
        (dst src thresh maxval) :=>
          dst :<- (threshold-binary src thresh maxval)
      }

#### Generated LLVM IR
| Type   | Single-core | Multi-core |
|--------|-------------|------------|
| u8CXY  | [Download](https://s3.amazonaws.com/liblikely/benchmarks/binary_threshold_u8CXY__u8CXY_double_double_.ll)   | [Download](https://s3.amazonaws.com/liblikely/benchmarks/binary_threshold_u8CXY__u8CXY_double_double__m.ll)   |
| f32CXY | [Download](https://s3.amazonaws.com/liblikely/benchmarks/binary_threshold_f32CXY__f32CXY_double_double_.ll) | [Download](https://s3.amazonaws.com/liblikely/benchmarks/binary_threshold_f32CXY__f32CXY_double_double__m.ll) |
