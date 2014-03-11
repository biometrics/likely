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

    complex = loop({ dzr = f32(x) / resolution - x_min,
                     dzi = f32(y) / resolution - y_min,
                     zr = f32(0),
                     zi = f32(0),
                     j  = u32(0),
                     it = u32(0) },
                   lt(j, iterations),
                   { j = j + 1,
                     zr = zr*zr - zi*zi + dzr,
                     zi = zr*zi*2 + dzi },
                   zr + zi
                  ){ columns = cols, rows = rows }
    show(complex)
