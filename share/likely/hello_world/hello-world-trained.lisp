d := (global "training-parameter" i32)

hello-world :=
  src :->
  {
    dst := src.imitate
    (dst src) :=>
      dst :<- (/ src d)
    dst
  }

(extern u8CXY "hello_world" u8CXY hello-world)
