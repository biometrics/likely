Average Face
------------
Compute the average face from a set of aligned faces [1].

    average :=
      data :->
      {
        dst := (new data.type data.channels data.columns data.rows 1 null)
        len := data.frames
        (dst data len) :=>
        {
          sum := (data.type.depth-double.depth-atleast-32 0).$
          (iter (-> t (<- sum (+ sum data)))
                len)
          dst :<- (/ sum len)
        }
      }

    average_face :=
      () :->
        [ "data/lfw2".read-directory-grayscale.average ]

Dynamic Execution

    (average_face)

Static Compilation

    (extern u8XY "likely_test_function" () average_face true)

[1] http://www.openu.ac.il/home/hassner/data/lfwa/
