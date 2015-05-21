### Householder tridiagonalization
Golub & Van Loan, "Matrix Computations 4th Edition", Section 8.3.1.

    householder-tridiagonalization-iteration :=
      (A k) :->
      {
        n := A.rows
        native-type := A.element-type

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
            (p i) :<- (* B (dot (-> j (A 0 (+ row-start j) (+ row-start i)))
                                (-> j (v j))
                                m
                                native-type))
        init-p.(iter m)

        v-norm := (dot (-> i (p i))
                       (-> i (v i))
                       m
                       native-type).(* B).(/ 2)

        w := ($ 0.native-type m)
        (-> i (<- (w i) (- (p i) (* v-norm (v i))))).(iter m)

        l2-norm := (norm-l2 (-> i (A 0 k (+ row-start i))) m native-type)
        (A 0 k row-start) :<- l2-norm
        (A 0 row-start k) :<- l2-norm
        set-zero :=
          i :->
          {
            (A 0 k i) :<- 0
            (A 0 i k) :<- 0
          }
        set-zero.(iter-range (+ k 2) n)

        update-A :=
          (x y) :->
          {
            ax := (+ row-start x)
            ay := (+ row-start y)
            val := (- (A 0 ax ay)
                      (+ (* (v x) (w y)) (* (v y) (w x))))
            (A 0 ax ay) :<- val
            (A 0 ay ax) :<- val
          }
        update-A.(iter-triangle m)
        A
      }

    householder-tridiagonalization :=
      src :->
      {
          ; Make a copy because we will iteratively modify the matrix in-place
          A := src.imitate
          (A src) :=>
            A :<- src

          ; Iteratively tridiagonalize the matrix
          (-> k (householder-tridiagonalization-iteration A k)).(iter (- A.rows 2))
          A
      }
