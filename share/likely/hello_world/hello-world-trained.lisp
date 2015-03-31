hello-world :=
  d :->
    src :->
    {
      dst := src.imitate
      (dst src) :=>
        dst :<- (/ src d)
      dst
    }

hello-world-trained :=
  d :->
    (extern u8CXY "hello_world" u8CXY (hello-world d))
