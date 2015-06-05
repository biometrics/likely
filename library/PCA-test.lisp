"library/PCA.md".import

mnist := "data/mnist/train-images-idx3-ubyte.lm".read-matrix
original-rows    := mnist.rows
original-columns := mnist.columns

mnist := (u8XY 1 (* mnist.columns mnist.rows) mnist.frames 1 mnist.data)

(PCA mnist)
