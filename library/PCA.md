Principal Component Analysis (PCA)
==================================

    "library/symmetric-QR.md".import

    PCA :=
      raw-samples :->
      {
        samples := raw-samples.(convert raw-samples.element-type.floating)
        mean := samples.average-row
        cov := samples.copy.(center mean).A-transpose-A.(scale (samples.element-type samples.rows).recip)
        evecs := cov
        evals := (symmetric-QR evecs)
        (eigen-sort evals evecs)
        evecs
      }
