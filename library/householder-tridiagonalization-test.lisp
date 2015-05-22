"library/householder-tridiagonalization.md".import
"library/transpose.md".import

src := (f32XY 1 4 4 1 (4.0 1 -2  2
                         1 2  0  1
                        -2 0  3 -2
                         2 1 -2 -1))

A1-truth := (f32XY 1 4 4 1 (4.0          3          0          0
                              3 (/ 10.0 3) (/ -4.0 3)         -1
                              1 (/ -4.0 3)         -1 (/ -4.0 3)
                             -1         -1 (/ -4.0 3)  (/ 5.0 3)))

T-truth := (f32XY 1 4 4 1 (4.0          3            0            0
                             3 (/ 10.0 3)    (/ 5.0 3)            0
                             0  (/ 5.0 3) (/ -33.0 25) (/ -68.0 75)
                             0          0 (/ -68.0 75) (/ 149.0 75)))

error-threshold := 0.000001

{
  A1 := src.copy
  (householder-tridiagonalization-iteration A1 0)
  (ensure-approximately-equal A1
                              A1-truth
                              error-threshold)

  Q1 := (householder-unfactor-matrix A1 1)
  (ensure-approximately-equal (matrix-multiplication Q1 Q1.transpose)
                              Q1.imitate.set-identity
                              error-threshold)

  ; Remove the encoded householder vector
  (A1 0 0 2) :<- 0
  (A1 0 0 3) :<- 0

  (ensure-approximately-equal (matrix-multiplication (matrix-multiplication Q1.transpose src) Q1)
                              A1
                              error-threshold)
}

{
  A := src.copy
  T := (householder-tridiagonalization A)
  (ensure-approximately-equal T T-truth error-threshold)

  Q := (householder-forward-accumulate A)

  (ensure-approximately-equal (matrix-multiplication Q Q.transpose)
                              Q.imitate.set-identity
                              error-threshold)
  ; T2 := (matrix-multiplication (matrix-multiplication Q.transpose src) Q)
  ; T2
  ; (ensure-approximately-equal T2
  ;                             T
  ;                            error-threshold)
}
