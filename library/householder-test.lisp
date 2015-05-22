"library/householder.md".import

src := (f32XY 1 4 4 1 (4.0 1 -2  2
                         1 2  0  1
                        -2 0  3 -2
                         2 1 -2 -1))

A0-truth := (f32XY 1 4 4 1 (4.0          3          0          0
                              3 (/ 10.0 3) (/ -4.0 3)         -1
                              1 (/ -4.0 3)         -1 (/ -4.0 3)
                             -1         -1 (/ -4.0 3)  (/ 5.0 3)))

T-truth := (f32XY 1 4 4 1 (4.0          3            0            0
                             3 (/ 10.0 3)    (/ 5.0 3)            0
                             0  (/ 5.0 3) (/ -33.0 25) (/ -68.0 75)
                             0          0 (/ -68.0 75) (/ 149.0 75)))

error-threshold := 0.000001

; Test a single iteration
{
  A0 := src.copy
  (householder-iteration A0 0)
  (ensure-approximately-equal A0
                              A0-truth
                              error-threshold)

  Q0 := (householder-unfactor A0 0)
  (ensure-approximately-equal (mtimes Q0 Q0.transpose)
                              Q0.imitate.set-identity
                              error-threshold)

  ; Remove the encoded householder vector
  (A0 0 0 2) :<- 0
  (A0 0 0 3) :<- 0

  (ensure-approximately-equal (mtimes (mtimes Q0.transpose src) Q0)
                              A0
                              error-threshold)
}

; Test a complete diagonalization
{
  Q := src.copy
  T := (householder Q)
  (ensure-approximately-equal T T-truth error-threshold)
  (ensure-approximately-equal (mtimes Q Q.transpose)
                              Q.imitate.set-identity
                              error-threshold)
  (ensure-approximately-equal (mtimes (mtimes Q.transpose src) Q)
                              T
                              error-threshold)
}
