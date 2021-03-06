### Convert grayscale
Compare to **[cv::cvtColor(CV_BGR2GRAY)](http://docs.opencv.org/modules/imgproc/doc/miscellaneous_transformations.html#cvtcolor)**.

    convert-grayscale :=
      src :->
      {
        (assume src.channels.(== 3))
        dst := (src.type.not-indirect.not-multi-channel 1 src.columns src.rows src.frames)
        src := (src.type.not-saturated src)
        (dst src) :=>
          dst :<- (bgr-to-y src.type (src 0) (src 1) (src 2))
      }

#### Generated LLVM IR
| Type    | Single-core | Multi-core |
|---------|-------------|------------|
| u8SCXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/convert_grayscale_u8SXY_u8SCXY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/convert_grayscale_u8SXY_u8SCXY_m.ll)   |
| u16SCXY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/convert_grayscale_u16SXY_u16SCXY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/convert_grayscale_u16SXY_u16SCXY_m.ll) |
| f32CXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/convert_grayscale_f32XY_f32CXY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/convert_grayscale_f32XY_f32CXY.ll)     |
