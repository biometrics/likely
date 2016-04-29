### Convolution
Compare to **[cv::filter2d](http://docs.opencv.org/2.4/modules/imgproc/doc/filtering.html#filter2d)**.

Pad a matrix with 0s along the X and Y axes

    pad-matrix :=
        (src pad) :->
        {
            padded := (src.type 1 (+ src.columns (* 2 pad))
                                  (+ src.rows    (* 2 pad))
                                     src.frames            ).set-zero
            (src padded pad) :=>
            {
                (padded 0 (+ pad x) (+ pad y) t) :<- src
            }
            padded
        }

Convolve a matrix with a kernel

    convolution :=
        (src kernel stride pad) :->
        {
            padded := (pad-matrix src pad)

            dst := (src.type 1 (/ (- padded.columns kernel.columns) stride).++
                               (/ (- padded.rows    kernel.rows   ) stride).++
                               (/ (- padded.frames  kernel.frames ) stride).++                  )

            (dst padded kernel stride) :=>
            {
                src-x := (* stride x)
                src-y := (* stride y)
                src-t := (* stride t)
                result := 0.f64.$
                (dst kernel padded src-x src-y src-t) :+>
                    result :<- (+ result (* (padded 0 (+ x src-x) (+ y src-y) (+ t src-t)) kernel))
                dst :<- result
            }
        }

        src := (f32XY 1 5 5 1 (1.f32 2 3 4 5
                               1     2 3 4 5
                               1     2 3 4 5
                               1     2 3 4 5
                               1     2 3 4 5))
        kernel := (f32XY 1 3 3 1 (2.f32 2 2
                                  2     2 2
                                  2     2 2))
        (convolution src kernel 1 0)

#### Generated LLVM IR
| Type   | Single-core | Multi-core |
|--------|-------------|------------|
| f32XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/match_template_f32XY__f32XY_f32XY_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/match_template_f32XY__f32XY_f32XY__m.ll) |
