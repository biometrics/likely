### Binary threshold
Compare to **[cv::threshold(THRESHOLD_BINARY)](http://docs.opencv.org/2.4.8/modules/imgproc/doc/miscellaneous_transformations.html#threshold)**.

    binary-threshold :=
      src :->
      {
        dst := src.imitate
        (dst src) :=>
          dst :<- (threshold-binary src 127 1)
      }
