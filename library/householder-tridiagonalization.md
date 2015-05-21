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

        x0 := (A 0 k row-start)
        u := (+ x0.sq sigma).sqrt
        B := (? (and (== sigma 0) (>= x0 0))
                0
             (? (and (== sigma 0) (< x0 0))
                -2
                {
                  v0 := (? (<= x0 0)
                           (- x0 u)
                           (- sigma).(/ (+ x0 u)))
                  norm-v :=
                    i :->
                      (v i) :<- (/ (v i) v0)
                  norm-v.(iter-range 1 m)
                  2.(* v0.sq).(/ (+ sigma v0.sq))
                }
             )).native-type.$

        w := ($ 0.native-type m)
        init-w :=
          i :->
            (w i) :<- (* B (dot (-> j (A 0 (+ row-start j) (+ row-start i))) v m))
        init-w.(iter m)
        v-norm := (dot w v m).(* B).(/ 2)
        (-> i (<- (w i) (- (w i) (* v-norm (v i))))).(iter m)

        ; Start modifying A
        (A 0 k row-start) :<- u
        (A 0 row-start k) :<- u

        ; Store the householder vector used to re-construct Q in the subdiagonal portion of A,
        ; and store zeros in the superdiagonal portion of A.
        store-householder-vector :=
          i :->
          {
            ai := (+ row-start i)
            (A 0 k ai) :<- (v i)
            (A 0 ai k) :<- 0
          }
        store-householder-vector.(iter-range 1 m)

        ; Symmetric update of the remainder of A
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
      A :->
      {
        ; Iteratively tridiagonalize the matrix in place
        (-> k (householder-tridiagonalization-iteration A k)).(iter (- A.rows 2))

        ; A now stores both the tridiagonal matrix and Q in factored form
        T := A.imitate
        (T A) :=>
          T :<- (? (<= (- x y).abs 1) A 0)
      }
