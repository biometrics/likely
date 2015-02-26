hello-world :=
  src :->
  {
    dst := src.imitate
    (dst src) :=>
      dst :<- (/ src 2)
    dst
  }

(extern u8CXY "hello_world" u8CXY hello-world)
