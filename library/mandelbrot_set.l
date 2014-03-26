Mandelbrot Set
--------------
[Wikipedia Explanation](http://en.wikipedia.org/wiki/Mandelbrot_set)

Control parameters

    width = mandelbrot_set_width ?? 600
    height = mandelbrot_set_height ?? 400
    x_min = -2
    x_max = 1
    y_min = -1
    y_max = 1
    iterations = 35

Definition

    mandelbrot_set = (=> ()
    {
      dzr = x.f32 / width * (x_max - x_min) + x_min
      dzi = y.f32 / height * (y_max - y_min) + y_min
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
    } ((columns width) (rows height)) )

Execution

    mandelbrot_set
