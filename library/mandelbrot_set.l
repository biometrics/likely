Mandelbrot Set
--------------
[Wikipedia Explanation](http://en.wikipedia.org/wiki/Mandelbrot_set)

Control parameters

    x_min = -2
    x_max = 1
    y_min = -1
    y_max = 1
    resolution = 128
    iterations = 35

Derived values

    cols = (x_max - x_min) * resolution + 1
    rows = (y_max - y_min) * resolution + 1

Initialization

    mandelbrot_set = (=> ()
    {
      dzr = x.f32 / resolution + x_min
      dzi = y.f32 / resolution + y_min
      zr = 0.f32
      zi = 0.f32
      j = 0
    loop :
      new_zr = zr * zr - zi * zi + dzr
      zi = zr * zi * 2 + dzi
      zr = new_zr
      j = j + 1
      (? (zr * zr + zi * zi < 2 * 2) & (j < iterations) loop)
      j
    } ((columns cols) (rows rows)) )

    mandelbrot_set
