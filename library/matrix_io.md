Matrix I/O
----------

    lenna := "data/misc/lenna.tiff".(read media image)
    encoded-lenna := lenna.(encode "jpg")
    decoded-lenna := encoded-lenna.(decode image)
    "Compression Ratio: " (/ decoded-lenna.bytes.f64 encoded-lenna.bytes.f64)
    ; decoded-lenna.(write "lenna.png")

[Previous](?href=comments) | [Next](?href=c_api)
