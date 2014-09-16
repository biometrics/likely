Matrix I/O
----------
There are 4 functions for matrix input and output:

| Function                      |
|-------------------------------|
| (**read** file_name)          |
| (**write** matrix file_name)  |
| (**encode** matrix extension) |
| (**decode** matrix)           |

    image:= "data/misc/lenna.tiff".read
    encoded:= image.(encode "jpg")
    (print "Compression Ratio: " image.bytes.f64:/ encoded.bytes)
    decoded:= encoded.decode
    ; decoded:write "lenna.png"

Supported file formats are:

| Extension                                                     | Type  |
|---------------------------------------------------------------|-------|
| [bmp](http://en.wikipedia.org/wiki/BMP_file_format)           | Image |
| [jpg](http://en.wikipedia.org/wiki/Jpg)                       | Image |
| [png](http://en.wikipedia.org/wiki/Portable_Network_Graphics) | Image |
| [tiff](http://en.wikipedia.org/wiki/Tagged_Image_File_Format) | Image |

**Image** formats expect single-frame matricies, **Video** formats (not supported yet) expect multi-frame matricies.

[Previous](?href=comments) | [Next](?href=c_api)
