### Householder tridiagonalization
See Golub & Van Loan, "Matrix Computations 4th Edition" (GVL).

GVL Algorithm 8.3.1

    householder-iteration :=
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

        ; Store the householder vector used to re-construct Q in the off-tridiagonal portion of A
        store-v :=
          i :->
          {
            ai := (+ row-start i)
            vi := (v i)
            (A 0 k ai) :<- vi
            (A 0 ai k) :<- 0
          }
        store-v.(iter-range 1 m)

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

GVL Section 5.1.6

    householder-unfactor :=
      (A h) :->
      {
        native-type := A.element-type
        j := h.++
        m := (- A.rows j)
        v := ($ 0.native-type m)
        (v 0) :<- 1
        u := 1.native-type.$
        init-vu :=
          i :->
          {
            e := (A 0 h (+ j i))
            (v i) :<- e
            u :<- (+ u e.sq)
          }
        init-vu.(iter-range 1 m)
        Bj := (/ 2 u)

        Q := A.imitate.set-identity
        update-Q :=
         (k i) :->
         {
           ji := (+ j i)
           jk := (+ j k)
           (Q 0 jk ji) :<- (- (Q 0 jk ji) Bj.(* (v i)).(* (v k)))
         }
        update-Q.(iter-square m)
        Q
      }

GVL Equation 5.1.5

    householder-backward-accumulation :=
      A :->
    {
      n := A.rows
      Q := A.imitate.set-identity

      ; Note the similarity between this function and "householder-unfactor",
      ; except that we don't explicitly form Qj.
      householder-backward-accumulation-iteration :=
        (A Q h) :->
        {
          native-type := A.element-type
          j := h.++
          m := (- n j)
          v := ($ 0.native-type m)
          (v 0) :<- 1
          u := 1.native-type.$
          init-vu :=
            i :->
            {
              e := (A 0 h (+ j i))
              (v i) :<- e
              u :<- (+ u e.sq)
            }
          init-vu.(iter-range 1 m)
          Bj := (/ 2 u)

          BjvTQ := ($ 0.native-type m)
          init-BjvTQ :=
            i :->
              (BjvTQ i) :<- (* Bj (dot v (-> k (Q 0 (+ j i) (+ j k))) m))
          init-BjvTQ.(iter m)

          update-Q :=
           (k i) :->
           {
             ji := (+ j i)
             jk := (+ j k)
             (Q 0 jk ji) :<- (- (Q 0 jk ji) (* (v i) (BjvTQ k)))
           }
          update-Q.(iter-square m)
        }
      (-> h (householder-backward-accumulation-iteration A Q h)).(iter-reverse (- n 2))
      Q
    }

GVL Section 8.3.1

    householder :=
      A :->
      {
        n := A.rows

        ; Iteratively tridiagonalize the matrix in place
        (-> k (householder-iteration A k)).(iter (- n 2))

        ; Note that A now stores both the tridiagonal matrix and Q in factored form
        T := A.imitate
        (T A) :=>
          T :<- (? (<= (- x y).abs 1) A 0)
        Q := (householder-backward-accumulation A)

        ; By convention A is set to Q and T is returned
        (set A Q)
        T
      }
