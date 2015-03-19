### Convert grayscale
Compare to **[cv::cvtColor(CV_BGR2GRAY)](http://docs.opencv.org/modules/imgproc/doc/miscellaneous_transformations.html#cvtcolor)**.

    convert-grayscale :=
      src :->
      {
        dst := (new src.type.not-multi-channel 1 src.columns src.rows src.frames null)
        (dst src) :=>
        {
          val := (bgr-to-y src.type (src 0) (src 1) (src 2))
          dst :<- (? dst.type.is-floating val (round-integer val dst.type))
        }
      }

#### Generated LLVM IR
| Type    | Single-core | Multi-core |
|---------|-------------|------------|
| u8SCXY  | [View](https://s3.amazonaws.com/liblikely/benchmarks/convert_grayscale_u8SCXY_u8SCXY.ll)   | [View](https://s3.amazonaws.com/liblikely/benchmarks/convert_grayscale_u8SCXY_u8SCXY_m.ll)   |
| u16SCXY | [View](https://s3.amazonaws.com/liblikely/benchmarks/convert_grayscale_u16SCXY_u16SCXY.ll) | [View](https://s3.amazonaws.com/liblikely/benchmarks/convert_grayscale_u16SCXY_u16SCXY_m.ll) |
| f32CXY  | [View](https://s3.amazonaws.com/liblikely/benchmarks/convert_grayscale_f32CXY_f32CXY.ll)   | [View](https://s3.amazonaws.com/liblikely/benchmarks/convert_grayscale_f32CXY_f32CXY.ll)     |
