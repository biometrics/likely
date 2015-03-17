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
| u8CXY  | [View](https://s3.amazonaws.com/liblikely/benchmarks/binary_threshold_u8CXY__u8CXY_u8_u8_.ll)     | [View](https://s3.amazonaws.com/liblikely/benchmarks/binary_threshold_u8CXY__u8CXY_u8_u8__m.ll)     |
| f32CXY | [View](https://s3.amazonaws.com/liblikely/benchmarks/binary_threshold_f32CXY__f32CXY_f32_f32_.ll) | [View](https://s3.amazonaws.com/liblikely/benchmarks/binary_threshold_f32CXY__f32CXY_f32_f32__m.ll) |
