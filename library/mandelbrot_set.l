Mandelbrot Set
--------------
[Wikipedia Explanation](http://en.wikipedia.org/wiki/Mandelbrot_set)

Control parameters

    x_min = -2
    x_max = 1
    y_min = -1
    y_max = 1
    resolution = 128
    iterations = 3

Derived values

    cols = (x_max - x_min) * resolution + 1
    rows = (y_max - y_min) * resolution + 1

Initialization

;   complex => (let (dzr = (cast x f32) / resolution - x_min
;                    dzi = (cast y f32) / resolution - y_min
;                    zr = (cast 0 f32)
;                    zi = (cast 0 f32)
;                    j  = (cast 0 u32)
;                    it = (cast 0 u32))
;                   (while (< j iterations)
;                          (j = j + 1
;                           zr = zr * zr - zi * zi + dzr
;                           zi = zr * zi * 2 + dzi
;                           zr + zi)))
;
; (complex (new f32 1 512 512))
