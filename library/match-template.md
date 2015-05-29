### Match template
Compare to **[cv::matchTemplate](http://docs.opencv.org/modules/imgproc/doc/object_detection.html#matchtemplate)**.

    match-template :=
      (src templ) :->
      {
        dst := (src.type 1 (- src.columns templ.columns).++
                           (- src.rows    templ.rows   ).++)
        (dst src templ) :=>
        {
          outer-x := x
          outer-y := y
          result := 0.f64.$
          (dst templ src outer-x outer-y) :+>
            result :<- (+ result (* (src 0 (+ x outer-x) (+ y outer-y)) (templ 0 x y)))
          dst :<- result
        }
      }

#### Generated LLVM IR
| Type   | Single-core | Multi-core |
|--------|-------------|------------|
| f32XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/match_template_f32XY__f32XY_f32XY_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/match_template_f32XY__f32XY_f32XY__m.ll) |
