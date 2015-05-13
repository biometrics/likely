Average Face
------------
Compute the average face from a set of aligned faces [1].

    average :=
      data :->
      {
        sum := (new data.type.depth-atleast-32.not-multi-frame data.channels data.columns data.rows 1 null)
        sum :=>
          sum :<- 0
        (sum data) :+>
          sum :<- (+ sum data)

        average := sum.(imitate-size data.type)
        norm := (/ 1 data.frames.floating)
        (average sum norm) :=>
          average :<- (* sum norm)
      }

    average_face :=
      () :->
        [ "data/lfw2".read-directory-grayscale.average ]

Dynamic Execution

    (average_face)

Static Compilation

    (extern u8XY "likely_test_function" () average_face true)

[1] http://www.openu.ac.il/home/hassner/data/lfwa/
