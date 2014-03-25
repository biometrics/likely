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

    mandelbrot_set = (=> ()
    {
      dzr = x.f32 / resolution - x_min
      dzi = y.f32 / resolution - y_min
      zr = 0.f32
      zi = 0.f32
      j = 0
    loop :
      zr = zr * zr - zi * zi + dzr
      zi = zr * zi * 2 + dzi
      j = j + 1
      (? (j < iterations) loop)
      zr + zi
    } ((columns cols) (rows rows)) )

    (mandelbrot_set)
