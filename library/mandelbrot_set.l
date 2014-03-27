Mandelbrot Set
--------------
[Wikipedia Explanation](http://en.wikipedia.org/wiki/Mandelbrot_set)

Reactive location and resoution

    x_min   = -2 + mandelbrot_set_x     ?? 0
    y_min   = -1 + mandelbrot_set_y     ?? 0
    x_range = 3  * mandelbrot_set_scale ?? 1
    y_range = 2  * mandelbrot_set_scale ?? 1
    width   = mandelbrot_set_width ?? 600
    height  = width / x_range * y_range
    iterations = 35

Definition

    mandelbrot_set = (=> ()
    {
      dzr = x.f32 / width * x_range + x_min
      dzi = y.f32 / height * y_range + y_min
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
