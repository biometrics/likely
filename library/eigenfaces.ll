Eigenfaces
---------
Compute eigenvectors from a set of aligned faces.

    avg = m =>
    {
      j = 0
      (j = j + m) $ t m.frames
      j / m.frames
    } : ((frames 1) (type parallel))

    faces = "../data/lfwa.tar.gz".read
    average_face = faces.avg
