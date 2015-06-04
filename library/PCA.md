Principal Component Analysis (PCA)
==================================

    "library/average.md".import
    "library/multiply-transposed.md".import
    "library/symmetric-QR.md".import

    PCA :=
      raw-samples :->
      {
        samples := raw-samples.(convert raw-samples.element-type.floating)
        mean := samples.average-row
        cov := (multiply-transposed samples mean)
        mean
      }
