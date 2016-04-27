Hello World
===========

Welcome to a brief tutorial on Likely!
Here we will cover the primary techniques for executing source code written in the Likely programming language.
Note that this document assumes a successful **[installation](?href=README.md)**.

For this tutorial we will consider a function called *hello-world* that takes one matrix as input, and returns as output a new matrix in which every element is half the value of the input matrix.
The Likely source code for this function is:

```lisp
hello-world :=             ; hello-world is
  src :->                  ; a function with one parameter "src"
  {                        ; which first
    dst := src.imitate     ; constructs a matrix "dst" with the same type and dimensionality as "src"
    (dst src) :=>          ; then every element in "dst"
      dst :<- (/ src 2)    ; is assigned a value equal to half the value of "src" at the same location
    dst                    ; finally "dst" is returned by the function.
  }
```

The remainder of this tutorial assumes that your working directory is the root of the Likely repository.

Likely as a Scripting Language
------------------------------
Copy the *hello-world* source code above into a file called *hello-world.lisp*.
Feel free to remove the comments (the semicolon through the end of the line) if they feel cluttered.

Let's start by running the file we just created:

```bash
$ likely hello-world.lisp
```

Hey, nothing happened!
Well that is because we defined a function but we didn't actually call it!
To call *hello-world* we need an image to provide as input.
Consider the famous _Lenna_:

```bash
$ likely -c '(read-image "data/misc/lenna.tiff")' -show
```

With the window that just popped up in focus, press any key to close it.
Note that *-c* runs a command instead of a file, and that *-show* displays the output in a window instead of printing it to the terminal.

We're now ready to run *hello-world* on *Lenna*. To do so, add the following line to the end of *hello-world.lisp*:

```lisp
(hello-world (read-image "data/misc/lenna.tiff")) ; call hello-world with Lenna
```

Let's re-run our code:

```bash
$ likely hello-world.lisp -show
```

Success!
The image we're seeing should look exactly twice as dark as the original *Lenna*.
To save our output instead of showing it:

```bash
$ likely hello-world.lisp -render dark_lenna_interpreted.png
```

Likely as a Just-In-Time Compiler
---------------------------------
Instead of using the provided *likely* executable as we did previously, this time we're interested in building our own application on top of the **[Likely API](https://s3.amazonaws.com/liblikely/doxygen/index.html)**.
We'd like for our application to just-in-time (JIT) compile the *hello-world* function source code into something callable from C.

Starting with our *hello-world.lisp* file from the previous section, replace the last line (calling hello-world with Lenna) with:

```lisp
(extern u8CXY "hello_world" u8CXY hello-world) ; Export a C function with a prototype:
                                               ;   likely_mat hello_world(likely_mat)
                                               ; and body:
                                               ;   hello-world
```

Note that *u8CXY* is part of the Likely type system, corresponding to an unsigned 8-bit multi-channel, multi-column, multi-row matrix.

The source code for our application is in **[share/likely/hello_world/hello_world_jit.c](share/likely/hello_world/hello_world_jit.c)**.
Notice that it expects three parameters: an input image, a source file, and an output image.
Its behavior is to read the input image, compile the source file, retrieve the compiled function, call the function with the input image to produce the output image, and finally save the output image.
Let's run it:

```
$ hello_world_jit data/misc/lenna.tiff hello-world.lisp dark_lenna_jit.png
Reading input image...
Dimensions: 512x512
Reading source code...
Creating a JIT compiler environment...
Compiling source code...
Calling compiled function...
Writing output image...
Cleaning up...
Done!
```

Success!
We can confirm that we get the exact same output as previously:

```bash
$ diff dark_lenna_jit.png dark_lenna_interpreted.png
```

Likely as a Static Compiler
---------------------------
Now we are interested in simplifying the application we just wrote.
Instead of waiting until runtime, we'd like to compile *hello-world* offline, ahead of time.

Using the same *hello-world.lisp* file from the previous section:

```bash
$ likely hello-world.lisp hello-world.o
```

We've just compiled our Likely source file into a native object file.
Now we can simplify the source code for our application to **[share/likely/hello_world/hello_world_static.c](share/likely/hello_world/hello_world_static.c)**.
Though the behavior is the same, notice it now expects only two parameters: an input image and an output image.
Let's run it:

```
$ hello_world_static data/misc/lenna.tiff dark_lenna_static.png
Reading input image...
Calling compiled function...
Writing output image...
Cleaning up...
```

Likely as a Pre-compiler
------------------------
Consider the situtation where we have a very long compilation process that involves a lot of machine learning, but where we would still like to realize the benifits of JIT compilation.
Instead of compiling to a native object file, you can instead compile to platform-independent LLVM bitcode.

Using the same *hello-world.lisp* file from the previous section:

```bash
$ likely hello-world.lisp hello-world.bc
```

We've just compiled our Likely source code into platform-independent LLVM bitcode.
We can load this bitcode at runtime using **[share/likely/hello_world/hello_world_precompiled.c](share/likely/hello_world/hello_world_precompiled.c)** and it will be JIT compiled for the host CPU.
Let's run it:

```
$ hello_world_precompiled data/misc/lenna.tiff hello-world.bc hello_world dark_lenna_precompiled.png
Reading input image...
Reading bitcode...
Compiling function...
Calling compiled function...
Writing output image...
Cleaning up...
```

Likely as a Standalone Language
-------------------------------
Hold on!
In all of the previous sections we still had to write some C code to execute our Likely function.
We'd like to write the *main* function in Likely instead of C, so that our application is written completely in Likely.

Starting with our *hello-world.lisp* file from the previous sections, replace the last line (exporting hello-world as "hello_world") with:

```lisp
main :=
  (argc argv) :->
  {
    (puts "Reading input image...")
    src := (argv 1).read-image
    (puts "Calling function...")
    dst := src.hello-world
    (puts "Writing output image...")
    (write dst (argv 2))
    0
  }

(extern int "main" (int string.pointer) main)
```

Then it's the same procedure to compile a native object file:

```bash
$ likely hello-world.lisp hello-world.o
```

Except this time the executable can be built without C code.
Let's run it:

```
$ hello_world_main data/misc/lenna.tiff dark_lenna_main.png
Reading input image...
Calling function...
Writing output image...
```
