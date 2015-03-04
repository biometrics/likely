Benchmark
---------
Likely functions measured against their [OpenCV](http://www.opencv.org) equivalents.

### Results
- [Single-core](https://s3.amazonaws.com/liblikely/benchmark/single-core.txt)
- [Multi-core](https://s3.amazonaws.com/liblikely/benchmark/multi-core.txt)

**Q:** Why are some of the multi-core benchmarks slower? <br>
**A:** For functions that are fast to execute, the overhead of multi-threaded execution isn't worth it.

### Functions
- [Binary threshold](?href=binary-threshold)
- [Fused multiply-add](?href=fused-multiply-add)
- [Minimum & maximum locations](?href=min-max-loc)
