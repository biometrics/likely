Average Face
------------
Compute the average face from a set of aligned faces [1].

    avg :=
      m :->
      {
        dst := (new m.type m.channels m.columns m.rows 1)
        len := m.frames
        (dst m len) :=>
        {
          (<- j 0)
          ($ (<- j (+ j (m c x y t))) t len)
          (<- dst (dst.type (/ j len)))
        }
      }

    average_face :=
      () :-> "data/lfw2".read-directory-grayscale.avg

[1] http://www.openu.ac.il/home/hassner/data/lfwa/
