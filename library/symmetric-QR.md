### Householder tridiagonalization
See Golub & Van Loan, "Matrix Computations 4th Edition" (GVL).

    "library/householder.md".import

GVL Algorithm 8.3.2

    implicit-symmetric-QR-step-with-Wilkinson-shift :=
      (dd dt p q) :->
    {
      0
    }

GVL Algorithm 8.3.3

    symmetric-QR :=
      A :->
    {
      Q := A.copy
      T := (householder Q)

      n := A.rows
      native-type := A.element-type

      ; Store T as a pair of n-vectors
      dd := ($ 0.native-type n)
      dt := ($ 0.native-type n)
      (dd 0) :<- (T)
      (dt 0) :<- 0
      init-n-vectors :=
        i :->
      {
        (dd i) :<- (T 0 i i)
        (dt i) :<- (T 0 i.-- i)
      }
      init-n-vectors.(iter-range 1 n)

      qq := 0.$
      iter :=
        () :->
      {
        tol :=
          x :->
            (/ x 100) ; A tolerance greater than unit roundoff.

        set-zero-small-tridiagonal-elements :=
          i :->
            (<= (dt i.++).abs (tol (+ (dd i).abs
                                      (dd i.++).abs))) :?
              (dt i.++) :<- 0
        set-zero-small-tridiagonal-elements.(iter n.--)

        p := 0.$
        q := 0.$
        (-> () (<- q q.++)).(while (-> () (&& (< q n)
                                               (not (dt (- n q.++))))))

        (< q n) :?
          (implicit-symmetric-QR-step-with-Wilkinson-shift dd dt p q)


        qq :<- qq.++
      }

      iter.(while (-> () (< qq n)))

      T
    }
