### Binary threshold
Compare to **[cv::threshold(THRESHOLD_BINARY)](http://docs.opencv.org/2.4.8/modules/imgproc/doc/miscellaneous_transformations.html#threshold)**.

    binary-threshold :=
      (src thresh maxval) :->
      {
        dst := src.imitate
        (dst src thresh maxval) :=>
          dst :<- (threshold-binary src thresh maxval)
      }
