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

            x1 := (dst 0 k row-start)
            B := (? (and (== sigma 0) (>= x1 0))
                    0
                 (? (and (== sigma 0) (< x1 0))
                    -2
                    {
                      u := x1.sq.(+ sigma).sqrt
                      v1 := (? (<= x1 0)
                               (- x1 u)
                               (- sigma).(/ (+ x1 u)))
                      v1-sq := v1.sq
                      B := 2.(* v1-sq).(/ (+ sigma v1-sq))
                      norm-v :=
                        i :->
                          (v i) :<- (/ (v i) v1)
                      norm-v.(iter-range 1 m)
                      B
                    }
                 )).native-type.$
          }

          tridiagonalization-iteration.(iter (- n 2))
          dst
      }
