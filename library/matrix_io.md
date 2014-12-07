Matrix I/O
----------

    lenna := "data/misc/lenna.tiff".read-image
    encoded-lenna := lenna.(encode "jpg")
    decoded-lenna := ((decode image) encoded-lenna)
    "Compression Ratio: " (/ decoded-lenna.bytes.f64 encoded-lenna.bytes.f64)
    ; decoded-lenna.(write "lenna.png")

[Previous](?href=comments) | [Next](?href=export)
