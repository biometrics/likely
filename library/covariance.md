### Covariance
Compare to **[cv::calcCovarMatrix](http://docs.opencv.org/modules/core/doc/operations_on_arrays.html#calccovarmatrix)**.

    covariance :=
      src :->
    {
      avg := (new src.type.depth-atleast-32.floating.not-multi-row src.channels src.columns 1 src.frames null)
      avg :=>
        avg :<- 0
      (avg src) :+>
        avg :<- (+ avg src)
      norm := (/ 1 src.rows.floating)
      (avg norm) :=>
        avg :<- (* avg norm)

      centered := src.(imitate-size avg.type)
      (centered src avg) :=>
        centered :<- (- src avg)

      cov := (new avg.type.multi-row src.channels src.columns src.columns src.frames null)
      len := src.rows
      (cov centered len) :=>
      {
        dot := 0.(cast cov).$
        (-> i (<- dot (+ dot (* (centered c x i t)
                                (centered c y i t))))).(iter len)
        cov :<- dot
      }

      cov
    }
