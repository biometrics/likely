"library/householder-tridiagonalization.md".import

src := (f32XY 1 4 4 1 (4.0 1 -2  2
                         1 2  0  1
                        -2 0  3 -2
                         2 1 -2 -1))

A1 := (f32XY 1 4 4 1 (4.0          3          0          0
                        3 (/ 10.0 3) (/ -4.0 3)         -1
                        1 (/ -4.0 3)         -1 (/ -4.0 3)
                       -1         -1 (/ -4.0 3)  (/ 5.0 3)))

A := (f32XY 1 4 4 1 (4.0          3            0            0
                       3 (/ 10.0 3)    (/ 5.0 3)            0
                       0  (/ 5.0 3) (/ -33.0 25) (/ -68.0 75)
                       0          0 (/ -68.0 75) (/ 149.0 75)))

error-threshold := 0.000001

(ensure-approximately-equal (householder-tridiagonalization-iteration src.copy 0) A1 error-threshold)
(ensure-approximately-equal (householder-tridiagonalization           src.copy  ) A  error-threshold)

; {
;   Q := src.copy
;   T1 := (householder-tridiagonalization Q)
;   T2 := (tridiagonal-decomposition src.copy Q)
;   T2
; }
