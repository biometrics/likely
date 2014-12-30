Average Face
------------
Compute the average face from a set of aligned faces [1].

    avg :=
      m :->
      {
        dst := (new m.type m.channels m.columns m.rows 1 null)
        len := m.frames
        (dst m len) :=>
        {
          j :<- (m.type.depth-double.depth-atleast-32 0)
          (-> t (<- j (+ j (m c x y t)))).(iter len)
          dst :<- (dst.type (/ j len))
        }
      }

    average_face :=
      () :-> "data/lfw2".read-directory-grayscale.avg

[1] http://www.openu.ac.il/home/hassner/data/lfwa/
