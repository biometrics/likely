Faces
-----

    lfwa = "../data/lfwa.tar.gz".read
    sum = m =>
      {
        j = 0
        (j = j + m) $ t m.frames
      } : (frames 1)
    (sum lfwa)
