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
| Type    | Single-core | Multi-core |
|---------|-------------|------------|
| u8SCXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/binary_threshold_u8SCXY__u8SCXY_u8S_u8S_.ll)     | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/binary_threshold_u8SCXY__u8SCXY_u8S_u8S__m.ll)     |
| i16SCXY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/binary_threshold_i16SCXY__i16SCXY_i16S_i16S_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/binary_threshold_i16SCXY__i16SCXY_i16S_i16S__m.ll) |
| f32CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/binary_threshold_f32CXY__f32CXY_f32_f32_.ll)     | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/binary_threshold_f32CXY__f32CXY_f32_f32__m.ll)     |
