Hello World
-----------
Click **View Source** and copy this source code into _dream_. **CTRL+scroll** to edit numerical constants, note that it is re-executed as you edit:

    (define lenna (read "../data/misc/lenna.tiff"))

    (define hello_world
      (kernel (a)
        (/ a (cast 2 (type a)))))

    (hello_world lenna)

[Next](?show=comments)
