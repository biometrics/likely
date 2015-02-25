Hello World
-----------

    lenna := "data/misc/lenna.tiff".read-image
    hello_world :=
      src :->
      {
        dst := src.imitate
        (dst src) :=>
          (<- dst (/ src (src.type 2)))
      }

Dynamic Execution

    (hello_world lenna)

Static Compilation

    (extern u8CXY "likely_test_function" u8CXY hello_world true)
