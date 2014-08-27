Matrix I/O
----------
There are 5 functions covering matrix creation, input, and output:

| Function                         | C API                                        |
|----------------------------------|----------------------------------------------|
| (**new** type channels columns rows frames data) | [likely_new](include/likely/likely_runtime.h) |
| (**read** file_name)          | [likely_read](include/likely/likely_io.h)   |
| (**write** matrix file_name)  | [likely_write](include/likely/likely_io.h)  |
| (**encode** matrix extension) | [likely_encode](include/likely/likely_io.h) |
| (**decode** matrix)           | [likely_decode](include/likely/likely_io.h) |

    image:= "data/misc/lenna.tiff".read
    encoded:= image.(encode "jpg")
    (print "Compression Ratio: " (scalar image.bytes.f64:/ encoded.bytes))
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
