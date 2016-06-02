### Match template
Compare to **[cv::matchTemplate](http://docs.opencv.org/modules/imgproc/doc/object_detection.html#matchtemplate)**.

    match-template :=
      (src templ) :->
      {
        width  := templ.columns
        height := templ.rows
        dst := (src.type.not-indirect 1 (- src.columns width ).++
                                        (- src.rows    height).++)
        (dst src templ width height) :=>
        {
          result := 0.f64.$
          dot :=
            (inner-x inner-y) :->
              result :<- (+ (result) (* (src 0 (+ x inner-x) (+ y inner-y)) (templ 0 inner-x inner-y)))
          (iter-rectangle dot width height)
          dst :<- (result)
        }
      }

#### Generated LLVM IR
| Type   | Single-core | Multi-core |
|--------|-------------|------------|
| f32XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/match_template_f32XY__f32XY_f32XY_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/match_template_f32XY__f32XY_f32XY__m.ll) |
