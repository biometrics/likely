Eigenfaces
---------
Compute eigenvectors from a set of aligned faces.

    mean:= (=> (x axis)
    {
      j:= 0
      ($ j:= j:+ x t x.axis)
      j:/ x.axis
    } (1.axis parallel.type))

    faces:= "../data/lfwa.tar.gz".read
    average_face:= faces:mean frames
