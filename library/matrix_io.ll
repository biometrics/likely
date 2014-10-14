Matrix I/O
----------

    lenna:= "data/misc/lenna.tiff".read
    encoded-lenna:= lenna.(encode "jpg")
    (string "Compression Ratio: " lenna.bytes.f64:/ encoded-lenna.bytes)
    decoded-lenna:= encoded-lenna.decode
    ; decoded-lenna.(write "lenna.png")

[Previous](?href=comments) | [Next](?href=c_api)
