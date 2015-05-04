### Matrix multiplication
Compare to **[A*B](http://docs.opencv.org/modules/core/doc/basic_structures.html#matrixexpressions)**.

    matrix-multiplication :=
      (A B) :->
      {
        (assume (== B.rows A.columns))
        C := (new A.type 1 B.columns A.rows 1 null)
        len := A.columns
        (C A B len) :=>
        {
          dot-product := 0.f64.$
          madd :=
            i :->
              dot-product :<- (+ dot-product (* (A 0 i y).f64 (B 0 x i).f64))
          (iter madd len)
          C :<- dot-product
        }
      }

#### Generated LLVM IR
| Type   | Single-core | Multi-core |
|--------|-------------|------------|
| f32XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/matrix_multiplication_f32XY__f32XY_f32XY_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/matrix_multiplication_f32XY__f32XY_f32XY__m.ll) |
| f64XY  | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/matrix_multiplication_f64XY__f64XY_f64XY_.ll) | [View](https://raw.githubusercontent.com/biometrics/likely/gh-pages/ir/benchmarks/matrix_multiplication_f64XY__f64XY_f64XY__m.ll) |
