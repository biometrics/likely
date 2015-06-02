### Householder tridiagonalization
See Golub & Van Loan, "Matrix Computations 4th Edition" (GVL).

    "library/householder.md".import

GVL Algorithm 8.3.2

    implicit-symmetric-QR-step-with-Wilkinson-shift :=
      (dd dt p q n Q) :->
    {
      native-type := Q.element-type
      begin := p
      end := (- n q)
      d := (/ (- (dd end.--) (dd end)) 2)
      u := (- (dd end)
              (/ (dt end).sq
                 (+ d
                    (* d.sign
                       (+ d.sq (dt end).sq).sqrt))))
      x := (- (dd begin) u).$
      z := (dt begin.++).$
      givens-rotation :=
        k :->
        {
          a := x
          b := z
          c := 1.native-type.$
          s := 0.native-type.$
          (!= b 0) :?
            (? (> b.abs a.abs)
               {
                 t := (/ (- a) b)
                 s :<- (+ 1 t.sq).sqrt.recip
                 c :<- (* s t)
               } {
                 t := (/ (- b) a)
                 c :<- (+ 1 t.sq).sqrt.recip
                 s :<- (* c t)
               })

          ; GVL Section 5.1.9
          apply-rows :=
            j :->
            {
              j
            }
          apply-rows.(iter-range begin end)

          apply-columns :=
            j :->
            {
              j
            }
          apply-columns.(iter-range begin end)

        }
      givens-rotation.(iter-range begin end)
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

      p := 0.$
      q := 0.$
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

        (-> () (<- p p.++)).(while (-> () (&& (< p n)
                                              (not (dt p.++)))))
        (-> () (<- q q.++)).(while (-> () (&& (< q n)
                                              (not (dt (- n q.++))))))

        (< q n) :?
          (implicit-symmetric-QR-step-with-Wilkinson-shift dd dt p q n Q)

        q :<- q.++
      }

      iter.(while (-> () (< q n)))

      T
    }
