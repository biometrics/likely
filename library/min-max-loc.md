### Minimum & maximum locations
Compare to **[cv::minMaxLoc](http://docs.opencv.org/2.4.8/modules/core/doc/operations_on_arrays.html#minmaxloc)**.

    min-max-loc :=
      src :->
      {
        dst := (new f64XY.(imitate-channel src.type).(imitate-frame src.type) src.channels 3 2 src.frames null)
        width := src.columns
        height := src.rows
        ((dst.channels 1 1 dst.frames) dst src width height) :=>
        {
          current-min-value :<~ dst.type.numeric-limit-max
          current-min-x     :<~ 0
          current-min-y     :<~ 0
          current-max-value :<~ dst.type.numeric-limit-min
          current-max-x     :<~ 0
          current-max-y     :<~ 0

          check-location :=
            (x y) :->
            {
              current-value := (src c x y t)
              (? (< current-value current-min-value)
              {
                current-min-value :<- current-value
                current-min-x :<- x
                current-min-y :<- y
              })

              (? (> current-value current-max-value)
              {
                current-max-value :<- current-value
                current-max-x :<- x
                current-max-y :<- y
              })
            }

          check-row :=
            y :-> (iter (-> x (check-location x y)) width)

          (iter (-> y (check-row y)) height)

          (dst c 0 0) :<- current-min-value
          (dst c 1 0) :<- current-min-x
          (dst c 2 0) :<- current-min-y
          (dst c 0 1) :<- current-max-value
          (dst c 1 1) :<- current-max-x
          (dst c 2 1) :<- current-max-y
        }
      }