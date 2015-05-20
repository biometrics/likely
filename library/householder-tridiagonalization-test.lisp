"library/householder-tridiagonalization.md".import

src := (f32XY 1 4 4 1 (4.0 1 -2  2
                         1 2  0  1
                        -2 0  3 -2
                         2 1 -2 -1))

src

(householder-tridiagonalization src)
