Mandelbrot Set
--------------
[Wikipedia Explanation](http://en.wikipedia.org/wiki/Mandelbrot_set)

Reactive location and resoution

    x_min   = -2 + mandelbrot_set_x     ?? 0
    y_min   = -1 + mandelbrot_set_y     ?? 0
    x_range = 3  * mandelbrot_set_scale ?? 1
    y_range = 2  * mandelbrot_set_scale ?? 1
    width   =      mandelbrot_set_width ?? 600
    height  = width / x_range * y_range
    iter    = 35

Definition

    mandelbrot_set = (=> ()
    {
      zr0 = x.f32 / width  * x_range + x_min
      zi0 = y.f32 / height * y_range + y_min
      zr  = 0.f32
      zi  = 0.f32
      j   = 0
    loop :
      tmp = zr * zr - zi * zi + zr0
      zi  = 2 * zr * zi       + zi0
      zr  = tmp
      j   = j + 1
      (? (zr * zr + zi * zi < 2 * 2) & (j < iter) loop)
      j
    } ((columns width) (rows height)) )

Execution

    mandelbrot_set
