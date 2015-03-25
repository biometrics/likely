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
    (puts "Reading input image...")
    src := (argv 1).read-image
    (puts "Calling function...")
    dst := src.hello-world
    (puts "Writing output image...")
    (write dst (argv 2))
    0
  }

(extern int "main" (int string.pointer) main)
