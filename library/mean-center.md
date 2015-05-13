### Mean center

    mean-center :=
      src :->
      {
        native-type := src.type.depth-atleast-32
        sum := (new native-type.not-multi-frame src.channels src.columns src.rows 1 null)
        sum :=>
          sum :<- 0
        (sum src) :+>
          sum :<- (+ sum src)

        dst-type := native-type.floating
        average := sum.(imitate-size dst-type)
        norm := (/ 1 src.frames.floating)
        (average sum norm) :=>
          average :<- (* sum norm)

        dst := src.(imitate-size dst-type)
        (dst src average) :=>
          dst :<- (- src average)
      }
