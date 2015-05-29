### Householder tridiagonalization
See Golub & Van Loan, "Matrix Computations 4th Edition" (GVL).

    "library/householder.md".import

GVL Algorithm 8.3.3

    symmetric-QR :=
      A :->
    {
      Q := A.copy
      D := (householder Q)

      n := A.rows
      q := 0.$
      iter :=
        () :->
      {
        tol :=
          x :->
            (/ x 100)
        set-zero-small-tridiagonal-elements :=
          i :->
            (<= (D 0 i i.++).abs (tol (+ (D 0 i i).abs
                                         (D 0 i.++ i.++).abs))) :?
            {
              (D 0 i i.++) :<- 0
              (D 0 i.++ i) :<- 0
            }
        set-zero-small-tridiagonal-elements.(iter n.--)

        qq := 0.$
        p := 0.$


        q :<- q.++
      }

      iter.(while (-> () (< q n)))

      D
    }
