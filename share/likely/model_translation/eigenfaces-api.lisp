; This source file expects an environment in which `mean` and `evecs` are defined

eigenfaces :=
  src :->
  {
    mean  := mean
    evecs := evecs
    (assume-same-dimensions src mean)

    centered := (imitate-size src mean.type)
    (centered src mean) :=>
      centered :<- (- src mean)

    projection := (mean.element-type.multi-frame 1 1 1 evecs.frames)
    projection.set-zero

    (projection evecs centered) :+>
      projection :<- (+ (* evecs centered) projection)
  }

main :=
  (argc argv) :->
  {
    (puts "Reading input image...")
    src := (argv 1).read-image
    (puts "Computing projection...")
    dst := src.eigenfaces
    (puts "Printing feature vector...")
    (puts dst.to-string.data)
    0
  }

(extern int "main" (int string.pointer) main)
