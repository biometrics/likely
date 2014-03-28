Mandelbrot Set
--------------
[Wikipedia Explanation](http://en.wikipedia.org/wiki/Mandelbrot_set)

Reactive location and resolution

    x_scale  = 3
    y_scale  = 2
    x_range  = x_scale * (mandelbrot_set_scale ?? 1).f32
    y_range  = y_scale * (mandelbrot_set_scale ?? 1).f32
    x_min    = x_scale * (mandelbrot_set_x ?? 0) - x_range / 2 - 0.5
    y_min    = y_scale * (mandelbrot_set_y ?? 0) - y_range / 2
    width    = mandelbrot_set_width ?? 600
    height   = width / x_scale * y_scale
    iter     = 20 * (1 + (mandelbrot_set_angle ?? 0).f32 / 90)

Definition

    mandelbrot_set = (=> ()
    {
      zr0 = x.f32 / width  * x_range + x_min
      zi0 = y.f32 / height * y_range + y_min
      zr  = 0.f32
      zi  = 0.f32
      j   = 0
    loop #
      tmp = zr * zr - zi * zi + zr0
      zi  = 2 * zr * zi       + zi0
      zr  = tmp
      j   = j + 1
      (? (zr * zr + zi * zi < 2 * 2) & (j < iter) loop)
      j
    } ((columns width) (rows height)) )

Execution

    mandelbrot_set
