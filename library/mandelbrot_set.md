Mandelbrot Set
--------------
[Video Walkthrough](https://www.youtube.com/watch?v=a_hz8wFACVM)

Interactive parameters

    x_scale := 3
    y_scale := 2
    x_range := (try [ mandelbrot_set_scale ] 1).(* x_scale)
    y_range := (try [ mandelbrot_set_scale ] 1).(* y_scale)
    x_min   := (try [ mandelbrot_set_x ] 0).(* x_scale).(- (/ x_range 2.f)).(- 0.5)
    y_min   := (try [ mandelbrot_set_y ] 0).(* y_scale).(- (/ y_range 2.f))
    width   := (try [ mandelbrot_set_width ] 600)
    height  := width.(* y_scale).(/ x_scale)
    iter    := (try [ mandelbrot_set_angle ] 0).(/  4).(+ 20)

Definition

    mandelbrot_set :=
      (width height x_min y_min x_range y_range iter) :->
      {
        dst := (u8XY 1 width height)
        (dst width height x_min y_min x_range y_range iter) :=>
        {
          zr0 := (/ (* x.f32 x_range) width)  :+ x_min
          zi0 := (/ (* y.f32 y_range) height) :+ y_min
          zr := 0.f32.$
          zi := 0.f32.$
          j := 0.$
          loop := #
          tmp := (+ (- zr.sq zi.sq) zr0)
          zi :<- (+ (* (* zr zi) 2) zi0)
          zr :<- tmp
          j :<- (+ j 1)
          (& (< (+ zr.sq zi.sq) 4) (< j iter)) :? loop
          dst :<- (* 255 j).(/ iter).u8
        }
      }

Dynamic Execution

    (mandelbrot_set width height x_min y_min x_range y_range iter)

Static Compilation

    (extern u8XY "likely_test_function" (i32 i32 f32 f32 f32 f32 i32) mandelbrot_set true)
