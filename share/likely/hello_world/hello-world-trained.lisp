; This source file expects an environment in which `training-parameter` is defined

hello-world :=
  src :->
  {
    dst := src.imitate
    (dst src) :=>
      dst :<- (/ src training-parameter)
    dst
  }

(extern u8CXY "hello_world" u8CXY hello-world)
