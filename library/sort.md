### Sort
Compare to **[cv::sort(CV_SORT_EVERY_ROW)](http://docs.opencv.org/modules/core/doc/operations_on_arrays.html#sort)**.

    sort :=
      src :->
      {
        len := src.rows
        ((src.channels 1 src.rows src.frames) src len) :=>
        {
          (selection-sort (-> i (src c i y t))
                          (-> (x y) (< x y))
                          (-> (i j) {
                                      tmp := (src c i y t)
                                      (src c i y t) :<- (src c j y t)
                                      (src c j y t) :<- tmp
                                    })
                          len)
        }
      }

#### Generated LLVM IR
| Type   | Single-core | Multi-core |
|--------|-------------|------------|
| u8SXY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/sort_u8SXY_u8SXY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/sort_u8SXY_u8SXY_m.ll)   |
| i16SXY | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/sort_i16SXY_i16SXY.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/sort_i16SXY_i16SXY_m.ll) |
| i32XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/sort_i32XY_i32XY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/sort_i32XY_i32XY_m.ll)   |
| f32XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/sort_f32XY_f32XY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/sort_f32XY_f32XY_m.ll)   |
| f64XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/sort_f64XY_f64XY.ll)   | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/sort_f64XY_f64XY_m.ll)   |
