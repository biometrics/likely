### Eigen
Compare to **[cv::eigen](http://docs.opencv.org/modules/core/doc/operations_on_arrays.html#eigen)**.

    eigen :=
      src :->
      {
          ; Make a copy because we will iteratively modify the matrix in-place
          dst := src.imitate
          (dst src) :=>
            dst :<- src

          n := dst.rows
          native-type := dst.type.element-type

          ; Householder Tridiagonalization
          tridiagonalization-iteration :=
            k :->
          {
            ; Householder Vector
            row-start := (+ k 1)
            m := (- n row-start)
            sigma := 0.native-type.$
            v := ($ 0.native-type m)
            (v 0) :<- 1
            init-v :=
              i :->
              {
                e := (dst 0 k (+ row-start i))
                sigma :<- (+ sigma e.sq)
                (v i) :<- e
              }
            init-v.(iter-range 1 m)
          }

          tridiagonalization-iteration.(iter (- n 2))
          dst
      }
