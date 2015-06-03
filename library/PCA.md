Principal Component Analysis (PCA)
==================================

    "library/average.md".import
    "library/multiply-transposed.md".import

    PCA :=
      raw-samples :->
      {
        samples := raw-samples.(convert raw-samples.element-type.floating)
        mean := (average samples)
        mean
      }
