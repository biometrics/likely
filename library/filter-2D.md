### Filter 2D
Compare to **[cv::filter2d](http://docs.opencv.org/2.4/modules/imgproc/doc/filtering.html#filter2d)**.

    filter-2D :=
        (src kernel) :->
        {
            (assume src.channels.is-one)

            width  := kernel.columns
            height := kernel.rows
            (assume width.is-odd)
            (assume height.is-odd)

            padded := (src.type 1 (+ src.columns width.-- )
                                  (+ src.rows    height.--)
                                     src.frames            ).set-zero

            pad-columns := (/ (- width  1) 2)
            pad-rows    := (/ (- height 1) 2)
            (src padded pad-columns pad-rows) :=>
                (padded 0 (+ x pad-columns) (+ y pad-rows)) :<- src

            dst := src.(imitate-size src.element-type.depth-atleast-32.floating)

            (dst padded kernel width height) :=>
            {
                result := (dst.type 0).$
                dot :=
                  (inner-x inner-y) :->
                    result :<- (+ (result) (* (padded 0 (+ x inner-x) (+ y inner-y)) (kernel 0 inner-x inner-y)))
                (iter-rectangle dot width height)
                dst :<- (result)
            }
        }

#### Generated LLVM IR
| Type   | Single-core | Multi-core |
|--------|-------------|------------|
| u8XY   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/filter_2D_f32XY__u8XY_f32XY_.ll)  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/filter_2D_f32XY__u8XY_f32XY__m.ll)  |
| i16XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/filter_2D_f32XY__i16XY_f32XY_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/filter_2D_f32XY__i16XY_f32XY__m.ll) |
| f32XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/filter_2D_f32XY__f32XY_f32XY_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/filter_2D_f32XY__f32XY_f32XY__m.ll) |
| f64XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/filter_2D_f32XY__f64XY_f32XY_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/filter_2D_f32XY__f64XY_f32XY__m.ll) |
