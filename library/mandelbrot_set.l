Mandelbrot Set
--------------
[Wikipedia Explanation](http://en.wikipedia.org/wiki/Mandelbrot_set)

Control parameters

    (define x_min -2)
    (define x_max 1)
    (define y_min -1)
    (define y_max 1)
    (define resolution 128)
    (iterations 3)

Derived values

    (define cols (+ (* (- x_max x_min) resolution) 1))
    (define rows (+ (* (- y_max y_min) resolution) 1))

Initialization

;    complex = loop({ dzr = f32(x) / resolution - x_min,
;                     dzi = f32(y) / resolution - y_min,
;                     zr = f32(0),
;                     zi = f32(0),
;                     j  = u32(0),
;                     it = u32(0) },
;                   lt(j, iterations),
;                   { j = j + 1,
;                     zr = zr*zr - zi*zi + dzr,
;                     zi = zr*zi*2 + dzi },
;                   zr + zi
;                  ){ columns = cols, rows = rows }
;    show(complex)
