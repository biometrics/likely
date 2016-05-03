Hello World
===========
Welcome to a brief tutorial on Likely!
Here we will cover the primary techniques for executing source code written in the Likely programming language.
Note that this document assumes a successful **[installation](?href=README.md)**.
The exectuables referenced in this document should exist in the *build* directory.
Commands should be run from the root of the source repository.

In this tutorial we will consider a function called *hello-world* that takes one matrix as input and returns a new matrix as output in which every element is half the value of the corresponding input element.
The Likely source code for this function is:

```lisp
hello-world :=          ; hello-world is
  src :->               ; a function with one parameter "src"
  {                     ; which first
    dst := src.imitate  ; constructs a matrix "dst" with the same type and dimensionality as "src"
    (dst src) :=>       ; then every element in "dst"
      dst :<- (/ src 2) ; is assigned a value equal to half the value of "src" at the corresponding location
    dst                 ; finally "dst" is returned by the function.
  }
```

Likely as an Interpreter
------------------------
Copy the *hello-world* source code above into a file called *hello-world.lisp*.
Feel free to remove the comments (the semicolon through the end of the line) if they feel cluttered.

Let's start by running the file we just created:

```bash
$ ./build/likely hello-world.lisp
```

Hey, nothing happened!
Well that's because we defined a function but didn't actually call it!
To call *hello-world* we need an image to provide as input.
Consider the famous _Lenna_:

```bash
$ ./build/likely -c '(read-image "data/misc/lenna.tiff")' -show
```

With the window that just popped up in focus, press any key to close it.
Note that *-c* runs a command instead of a file, and that *-show* displays the output in a window instead of printing it to the terminal.

To run *hello-world* on *Lenna* add the following line to the end of *hello-world.lisp*:

```lisp
(hello-world (read-image "data/misc/lenna.tiff")) ; call hello-world with Lenna
```

Now let's re-run our code (switching to an identical version included in the Likely repository):

```bash
$ ./build/likely share/likely/hello_world/hello-world-interpreted.lisp -show
```

Success!
The image shown should look exactly twice as dark as the original *Lenna*.
To save our output instead of showing it:

```bash
$ ./build/likely share/likely/hello_world/hello-world-interpreted.lisp -render dark_lenna_interpreted.png
```

Likely as a Just-In-Time Compiler
---------------------------------
Instead of using the provided *likely* executable as we did previously, this time let's build our own application on top of the **[Likely API](https://s3.amazonaws.com/liblikely/doxygen/index.html)**.
We'd like for our application to just-in-time (JIT) compile the *hello-world* function source code into something callable from C.

Starting with our source file from the previous section, replace the last line (calling hello-world with Lenna) with:

```lisp
(extern u8CXY "hello_world" u8CXY hello-world) ; Export a C function with a prototype:
                                               ;   likely_mat hello_world(likely_mat)
                                               ; and body:
                                               ;   hello-world
```

Note that *u8CXY* is part of the Likely type system, indicating an unsigned 8-bit multi-channel, multi-column, multi-row matrix.

The source code for our application is in **[share/likely/hello_world/hello_world_jit.c](share/likely/hello_world/hello_world_jit.c)**.
The application expects three parameters: an input image, a source file, and an output image.
Its behavior is to read the input image, compile the source file, retrieve the compiled function, call the function with the input image to produce the output image, and finally save the output image.
Let's run it:

```
$ ./build/hello_world_jit data/misc/lenna.tiff share/likely/hello_world/hello-world-compiled.lisp dark_lenna_jit.png
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
Let's simplify the application we just wrote.
Instead of waiting until runtime, we'd like to compile *hello-world* offline, ahead of time.

Using the same source file from the previous section, compile it into a native object file:

```bash
$ ./build/likely share/likely/hello_world/hello-world-compiled.lisp hello-world.o
```

Now we can simplify the source code for our application to **[share/likely/hello_world/hello_world_static.c](share/likely/hello_world/hello_world_static.c)**.
Though the behavior is the same, notice that the application now expects only two parameters: an input image and an output image.
Let's run it:

```
$ ./build/hello_world_static data/misc/lenna.tiff dark_lenna_static.png
Reading input image...
Calling compiled function...
Writing output image...
Cleaning up...
```

Likely as a Pre-compiler
------------------------
Consider the situtation where we have a very long compilation process that involves a lot of machine learning, but where we would still like to realize the benifits of JIT compilation.
Instead of compiling to a native object file, you can instead compile to platform-independent LLVM bitcode.

Using the same file from the previous section, compiled it into platform-independent LLVM bitcode:

```bash
$ ./build/likely share/likely/hello_world/hello-world-compiled.lisp hello-world.bc
```

We can load this bitcode at runtime using **[share/likely/hello_world/hello_world_precompiled.c](share/likely/hello_world/hello_world_precompiled.c)** and it will be JIT compiled for the host CPU.
Let's run it:

```
$ ./build/hello_world_precompiled data/misc/lenna.tiff hello-world.bc hello_world dark_lenna_precompiled.png
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
We'd like to write the *main* function in Likely instead of C, so that our application is written entirely in Likely.

Starting with our source file from the previous sections, replace the last line (exporting hello-world as "hello_world") with:

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
$ ./build/likely share/likely/hello_world/hello-world-main.lisp hello-world-main.o
```

Then use the system linker to assemble the executable:

```
$ gcc hello-world-main.o -L build -llikely -o build/hello_world_main
```

Notice how the executable was built with just the one object file originating from the Likely source file.
Let's run it:

```bash
$ ./build/hello_world_main data/misc/lenna.tiff dark_lenna_main.png
Reading input image...
Calling function...
Writing output image...
```

Conclusion
----------
At this point we have covered the primary ways that Likely can compile and execute code.
If you are wondering how this relates to the project's goal of proving a framework for machine learning model translation and compilation then please proceed to the next tutorial on **[model translation](?href=share/likely/model_translation/README.md)**.
