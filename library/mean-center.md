### Mean center

    mean-center :=
      src :->
      {
        sum := (new src.type.depth-double.depth-atleast-32.not-multi-frame src.channels src.columns src.rows 1 null)
        sum :=>
          sum :<- 0
        (sum src) :+>
          sum :<- (+ sum src)

        average := sum.(imitate-size src.type)
        norm := (/ 1 src.frames.floating)
        (average sum norm) :=>
          average :<- (* sum norm)

        dst := src.(imitate-size src.type.depth-atleast-32.floating)
        (dst src average) :=>
          dst :<- (- src average)
      }
