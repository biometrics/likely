mean  := [ "data/demo/lfwa_grayscale_mean.lm".read-matrix  ]
evecs := [ "data/demo/lfwa_grayscale_evecs.lm".read-matrix ]

eigenfaces :=
  src :->
  {
    ; (assume-same-dimensions src mean)
    centered := (imitate-size src f32)
    ; (centered src mean) :=>
    ;  centered :<- (- src.f32 mean)
    centered
  }

main :=
  (argc argv) :->
  {
    (puts "Reading input image...")
    src := (argv 1).read-image
    (puts "Computing projection...")
    dst := src.eigenfaces
    0
  }

(extern int "main" (int string.pointer) main)
