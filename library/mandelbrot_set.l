Mandelbrot Set
--------------
[Video Walkthrough](https://www.youtube.com/watch?v=a_hz8wFACVM)

Responsive location and resolution

    x_scale  = 3
    y_scale  = 2
    x_range  = x_scale * (mandelbrot_set_scale ?? 1).f32
    y_range  = y_scale * (mandelbrot_set_scale ?? 1).f32
    x_min    = x_scale * (mandelbrot_set_x ?? 0).f32 - x_range / 2 - 0.5
    y_min    = y_scale * (mandelbrot_set_y ?? 0).f32 - y_range / 2
    width    = mandelbrot_set_width ?? 600
    height   = width / x_scale * y_scale
    iter     = 20 + (mandelbrot_set_angle ?? 0).i32 / 4

Definition

    mandelbrot_set = () =>
    {
      zr0 = x.f32 * x_range / width  + x_min
      zi0 = y.f32 * y_range / height + y_min
      zr  = 0.f32
      zi  = 0.f32
      j   = 0
    loop #
      tmp = zr * zr - zi * zi + zr0
      zi  = zr * zi * 2       + zi0
      zr  = tmp
      j   = j + 1
      ((zr * zr + zi * zi < 4) & (j < iter)) ? loop
      (255 * j / iter).u8
    } : ((channels 3) (columns width) (rows height) (type parallel))

Execution

    mandelbrot_set
