### Mean center

    mean-center :=
      src :->
      {
        dst := src.(imitate-size src.type.depth-atleast-32.floating)
        len := src.frames
        ((src.channels src.columns src.rows 1) dst src len) :=>
        {
          sum := (src.type.depth-atleast-32 0).$
          (-> t (<- sum (+ sum src))).(iter len)
          dst-type := dst.type
          beta := (/ sum.dst-type len.dst-type)
          (-> t (<- dst (- src.dst-type beta))).(iter len)
        }
      }
