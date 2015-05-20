### Match template
Compare to **[cv::matchTemplate](http://docs.opencv.org/modules/imgproc/doc/object_detection.html#matchtemplate)**.

    match-template :=
      (src templ) :->
      {
        dst := (src.type 1 (+ (- src.columns templ.columns) 1)
                           (+ (- src.rows    templ.rows   ) 1))
        width  := templ.columns
        height := templ.rows
        (dst src templ width height) :=>
        {
          result := 0.f64.$
          madd :=
            (i j) :->
              result :<- (+ result (* (src 0 (+ j x) (+ i y)) (templ 0 j i)))
          (iter (-> i (iter (-> j (madd i j)) width)) height)
          dst :<- result
        }
      }

#### Generated LLVM IR
| Type   | Single-core | Multi-core |
|--------|-------------|------------|
| f32XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/match_template_f32XY__f32XY_f32XY_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/match_template_f32XY__f32XY_f32XY__m.ll) |
