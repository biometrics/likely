### Fused multiply-add
Compare to **[cv::Mat::convertTo](http://docs.opencv.org/2.4.8/modules/core/doc/basic_structures.html#mat-convertto)**.

    fused-multiply-add :=
      (src alpha beta) :->
      {
        dst-type := src.type.floating
        dst := src.(imitate-size dst-type)
        (dst src alpha beta) :=>
          dst :<- (+ (* src.dst-type alpha.dst-type) beta.dst-type)
      }
