Benchmark
---------
Likely functions tested against their [OpenCV](http://www.opencv.org) equivalents.

**[Results](https://s3.amazonaws.com/liblikely/benchmark.txt)**

### Fused multiply add
Compare to **[cv::Mat::convertTo](http://docs.opencv.org/2.4.8/modules/core/doc/basic_structures.html#mat-convertto)**.

    fused-multiply-add :=
      src :->
      {
        dst-type := src.type.floating
        dst := src.(imitate-size dst-type)
        (dst src) :=>
          (<- dst (+ (* src.dst-type 2.dst-type) 3.dst-type))
      }

### Binary threshold
Compare to **[cv::threshold(THRESHOLD_BINARY)](http://docs.opencv.org/2.4.8/modules/imgproc/doc/miscellaneous_transformations.html#threshold)**.

    binary-threshold :=
      src :->
      {
        dst := src.imitate
        (dst src) :=>
          (<- dst (src.type (threshold-binary src 127 1)))
      }

### Minimum & Maximum locations
Compare to **[cv::minMaxLoc](http://docs.opencv.org/2.4.8/modules/core/doc/operations_on_arrays.html#minmaxloc)**.

    min-max-loc :=
      src :->
      {
        dst := (new f64C 6 1 1 1 null)
      }
