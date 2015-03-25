hello-world :=
  src :->
  {
    dst := src.imitate
    (dst src) :=>
      dst :<- (/ src 2)
    dst
  }

main :=
  (argc argv) :->
  {
    dst := (argv 1).read-image.hello-world
    (? (> argc 2) (write dst (argv 2))
                  (show dst "hello world!"))
    0
  }

(extern int "main" (int string.pointer) main)
