Benchmarks
----------
Likely functions measured against their [OpenCV](http://www.opencv.org) equivalents.

### Results
- [Single-core](https://s3.amazonaws.com/liblikely/single-core.txt)
- [Multi-core](https://s3.amazonaws.com/liblikely/multi-core.txt)

### FAQ
**Q:** Why are some of the multi-core benchmarks slower? <br>
**A:** For functions that are fast to execute, the overhead of multi-threaded execution isn't worth it.

**Q:** Where can I look up the definition of an operand? <br>
**A:** In the [standard library](https://s3.amazonaws.com/liblikely/latex/standard.pdf).

### Functions
- [Binary threshold](?href=binary-threshold)
- [Convert grayscale](?href=convert-grayscale)
- [Multiply-add](?href=multiply-add)
- [Minimum & maximum locations](?href=min-max-loc)
- [L2 normalization](?href=normalize-l2)
- [Matrix multiplication](?href=matrix-multiplication)
- [GEMM](?href=gemm)
