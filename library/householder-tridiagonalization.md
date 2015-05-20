### Householder tridiagonalization
Golub & Van Loan, "Matrix Computations 4th Edition", Section 8.3.1.

    householder-tridiagonalization :=
      src :->
      {
          ; Make a copy because we will iteratively modify the matrix in-place
          A := src.imitate
          (A src) :=>
            A :<- src

          n := A.rows
          native-type := A.element-type

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
                e := (A 0 k (+ row-start i))
                sigma :<- (+ sigma e.sq)
                (v i) :<- e
              }
            init-v.(iter-range 1 m)

            x1 := (A 0 k row-start)
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

            p := ($ 0.native-type m)
            init-p :=
              i :->
              {
                dot := 0.native-type.$
                row := (+ row-start i)
                (-> j (<- dot (+ dot (* (A 0 (+ row-start j) row) (v j))))).(iter m)
                (p i) :<- (* B dot)
              }
            init-p.(iter m)

            v-norm := 0.native-type.$
            (-> i (<- v-norm (+ v-norm (* (p i) (v i))))).(iter m)
            v-norm := v-norm.(* B).(/ 2)

            w := ($ 0.native-type m)
            (-> i (<- (w i) (- (p i) (* v-norm (v i))))).(iter m)

            l2-norm := 0.native-type.$
            (-> i (<- l2-norm (+ l2-norm (A 0 k (+ row-start i))))).(iter m)
            l2-norm := l2-norm.sqrt

            (A 0 k row-start) :<- l2-norm
            (A 0 row-start k) :<- l2-norm

            ((1 m m) A v w row-start) :=>
              (A 0 (+ row-start x) (+ row-start y)) :<- (- (A 0 (+ row-start x) (+ row-start y))
                                                        (+ (* (v x) (w y)) (* (v y) (w x))))
          }

          tridiagonalization-iteration.(iter (- n 2))
          A
      }
