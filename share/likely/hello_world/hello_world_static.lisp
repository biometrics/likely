(extern f32X "hello_world_div2" f32X
  a :->
  {
    dst := a.imitate
    (dst a) :=> (<- dst (/ a (a.type 2)))
  }
)
