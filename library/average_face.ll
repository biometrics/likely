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
          (<- j (m.type.native 0))
          ($ (<- j (+ j m)) t len)
          (<- dst (dst.type (/ j len)))
        }
      }

    (average_face) := () :-> "data/lfw2".read.avg
    (average_face)

[1] http://www.openu.ac.il/home/hassner/data/lfwa/
