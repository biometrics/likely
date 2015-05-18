### Eigen
Compare to **[cv::eigen](http://docs.opencv.org/modules/core/doc/operations_on_arrays.html#eigen)**.

    eigen :=
      src :->
      {
        native-type := src.type.element-type

        ; Householder Tridiagonalization
        n := src.rows

        iteration :=
          k :->
        {
          m := (- n (+ k 1))
          sigma := 0.native-type.$
          compute-sigma :=
            q :->
              sigma :<- (+ sigma q)
          compute-sigma.(iter (- m 1))
          v := ($ 0 1)
        }

        iteration.(iter (- n 2))
        src
      }
