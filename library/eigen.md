### Eigen
Compare to **[cv::eigen](http://docs.opencv.org/modules/core/doc/operations_on_arrays.html#eigen)**.

    eigen :=
      src :->
      {
        native-type := src.type.element-type

        ; Make a copy because we will iteratively modify the matrix in-place
        dst := src.imitate
        (dst src) :=>
          dst :<- src

        ; Householder Tridiagonalization
        n := dst.rows

        iteration :=
          k :->
        {
          m := (- n (+ k 1))
          sigma := 0.native-type.$
          (-> q (<- sigma (+ sigma q))).(iter (- m 1))
          v := ($ 0.native-type m)
        }

        iteration.(iter (- n 2))
        dst
      }
