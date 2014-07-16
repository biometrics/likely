Mandelbrot Set
--------------
[Video Walkthrough](https://www.youtube.com/watch?v=a_hz8wFACVM)

Responsive location and resolution

    x_scale  = 3
    y_scale  = 2
    x_range  = x_scale * mandelbrot_set_scale ?? 1
    y_range  = y_scale * mandelbrot_set_scale ?? 1
    x_min    = x_scale * mandelbrot_set_x ?? 0 - x_range / 2.f - 0.5
    y_min    = y_scale * mandelbrot_set_y ?? 0 - y_range / 2.f
    width    = mandelbrot_set_width ?? 600
    height   = width * y_scale / x_scale
    iter     = 20 + mandelbrot_set_angle ?? 0 / 4

Definition

    (mandelbrot_set u32 u32 f32 f32 f32 f32 u32) =
      (width height x_min y_min x_range y_range iter) =>
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
      result = (255 * j / iter).u8
      (result result result)
    } : ((columns width) (rows height) (type parallel))

Execution

    (mandelbrot_set width height x_min y_min x_range y_range iter)

Command Line

```bash
$ likely ../library/mandelbrot_set.ll -gui
$ likely ../library/mandelbrot_set.ll -record mandelbrot_set.png
$ dream ../library/mandelbrot_set.ll
```

[Next](?show=average_face)
