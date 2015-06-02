"library/symmetric-QR.md".import

error-threshold := 0.00001

; Test a single Givens Rotation
{
  A := (f32XY 1 3 3 1 (6.0 5 0
                         5 1 4
                         0 4 3))

  A1-truth := (f32XY 1 3 3 1 (7.8102.f32  4.4813 2.5607
                                       0 -2.4327 3.0729
                                       0       4      3))

  r := (+ (A 0 0 0).sq (A 0 0 1).sq).sqrt
  c := (/ (A 0 0 0) r)
  s := (/ (- (A 0 0 1)) r)
  A1 := (apply-givens-GTA A.copy 0 3 0 1 c s)

  (ensure-approximately-equal A1 A1-truth error-threshold)
}

; Test complete Symmetric QR
{
  src := (f32XY 1 4 4 1 (4.0 1 -2  2
                           1 2  0  1
                          -2 0  3 -2
                           2 1 -2 -1))

  D-truth := (f32XY 1 4 4 1 (6.84462.f32        0       0       0
                                       0 -2.19752       0       0
                                       0        0 1.08437       0
                                       0        0       0 2.26853))

  Q := src.copy
  D := (symmetric-QR src)
  (ensure-approximately-equal D D-truth error-threshold)
  ; (ensure-approximately-equal (mtimes Q Q.transpose)
  ;                             Q.imitate.set-identity
  ;                             error-threshold)
  ; (ensure-approximately-equal (mtimes (mtimes Q.transpose src) Q)
  ;                             D
  ;                             error-threshold)
}
