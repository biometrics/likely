### Householder tridiagonalization
See Golub & Van Loan, "Matrix Computations 4th Edition" (GVL).

    "library/householder.md".import

GVL Section 5.1.9

    apply-givens-GTA :=
      (A p q k c s) :->
      {
        rotate :=
          j :->
          {
            t1 := (A 0 j k   )
            t2 := (A 0 j k.++)
            (A 0 k j)    :<- (- (* c t1) (* s t2))
            (A 0 k.++ j) :<- (+ (* s t1) (* c t2))
          }
        rotate.(iter-range p q)
      }

    apply-givens-AG :=
      (A p q k c s) :->
      {
        rotate :=
          j :->
          {
            t1 := (A 0 k    j)
            t2 := (A 0 k.++ j)
            (A 0 k j)    :<- (- (* c t1) (* s t2))
            (A 0 k.++ j) :<- (+ (* s t1) (* c t2))
          }
        rotate.(iter-range p q)
      }

GVL Algorithm 8.3.2

    implicit-symmetric-QR-step-with-Wilkinson-shift :=
      (T Q p q) :->
    {
      tnn     := (T 0 q    q   )
      tnn-1   := (T 0 q    q.--)
      tn-1n-1 := (T 0 q.-- q.--)

      d := (- tn-1n-1 tnn).(/ 2)
      u := (- tnn
              (/ tnn-1.sq
                 (+ d
                    (* d.sign
                       (+ d.sq tnn-1.sq).sqrt))))

      x := (T 0 p p).(- u).$
      z := (T 0 p p.++).$

      native-type := Q.element-type

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

          (apply-givens-GTA T p q k c s)
          (apply-givens-AG  T p q k c s)
          (apply-givens-AG  Q p q k c s)

          (< k q.--) :?
          {
            x :<- (T 0 k (+ k 1))
            z :<- (T 0 k (+ k 2))
          }
        }
      givens-rotation.(iter-range p q)
    }

GVL Algorithm 8.3.3

    symmetric-QR :=
      A :->
    {
      Q := A.copy
      T := (householder Q)

      n := A.rows
      p := 0.$
      q := (- n 1).$

      symmetric-QR-iteration :=
        () :->
      {
        tol :=
          x :->
            (/ x 100) ; A tolerance greater than unit roundoff.

        set-zero-small-tridiagonal-elements :=
          i :->
            (<= (T 0 i i.++).abs (tol (+ (T 0 i    i   ).abs
                                         (T 0 i.++ i.++).abs))) :?
              {
                (T 0 i i.++) :<- 0
                (T 0 i.++ i) :<- 0
              }
        set-zero-small-tridiagonal-elements.(iter-range p q)

        (-> () (<- p p.++)).(while (-> () (&& (< p n) (not (T 0 p p.++)))))
        (-> () (<- q q.--)).(while (-> () (&& (> q 0) (not (T 0 q q.--)))))

        (> q 0) :?
          (implicit-symmetric-QR-step-with-Wilkinson-shift T Q p q)
      }
      symmetric-QR-iteration.(while (-> () (> q 0)))

      T
    }
