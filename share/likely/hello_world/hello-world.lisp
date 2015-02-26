hello-world :=
  src :->
  {
    dst := src.imitate
    (dst src) :=>
      dst :<- (/ src 2)
    dst
  }

(hello-world (read-image "data/misc/lenna.tiff"))
